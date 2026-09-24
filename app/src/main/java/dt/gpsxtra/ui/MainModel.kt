package dt.gpsxtra.ui

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import dt.gpsxtra.DEFAULT_XTRA_URL
import dt.gpsxtra.PreferencesDataStore
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.net.URL

const val TAG = "MainModel"

class MainModel(application: Application) : AndroidViewModel(application)
{
	private val prefsKv = PreferencesDataStore(application.applicationContext)
	private val debugEvents = MutableSharedFlow<String>(extraBufferCapacity = 64) // neeed the buffer capacity or everything disappears

	data class MainState(
		val xtraUrl: String = DEFAULT_XTRA_URL,
		val lastRun: String = "2026-09-21 18:41:44.000", // when the android studio project was created
		val debugText: String = "",
		val isRunning: Boolean = false,
		val revertUrl: String = ""
	)

	private val _uiState = MutableStateFlow(MainState())
	val uiState = _uiState.asStateFlow()

	init
	{
		viewModelScope.launch()
		{
			val initialXtraUrl = prefsKv.xtraUrlFlow().first()
			val previousRun = prefsKv.lastRunFlow().first()
			_uiState.update() { currentState -> currentState.copy(xtraUrl = initialXtraUrl, revertUrl = initialXtraUrl, lastRun = previousRun) }

			combine(prefsKv.xtraUrlFlow(), prefsKv.lastRunFlow()) { (xtraUrl, lastRun) -> Pair(xtraUrl, lastRun) }
				.collect() { (xtraUrl, lastRun) -> _uiState.update { currentState -> currentState.copy(xtraUrl = xtraUrl, lastRun = lastRun)
					}
				}
		}
		viewModelScope.launch()
		{
			debugEvents.collect() { newText ->
				_uiState.update() { currentState -> currentState.copy(debugText = "${currentState.debugText}${newText}\n") }
				Log.i(TAG, newText)
			}
		}
	}


	fun appendDebug(newText: String)
	{
		debugEvents.tryEmit(newText)
	}

	fun injectXtra()
	{
		updateIsRunning(true)
		viewModelScope.launch()
		{
			val downloadOk = downloadXtra()
			if(!downloadOk)
			{
				appendDebug("Failed to download xtra data")
				updateIsRunning(false)
				return@launch
			}

			appendDebug("Would run C code now")
		}
	}

	private fun updateIsRunning(isRunning: Boolean)
	{
		_uiState.update() { currentState -> currentState.copy(isRunning = isRunning) }
	}

	fun updateUrl(newUrl: String)
	{
		_uiState.update() { currentState -> currentState.copy(xtraUrl = newUrl)}
	}

	fun saveUrl()
	{
		viewModelScope.launch() {
			val newUrl = _uiState.value.xtraUrl
			prefsKv.saveXtraUrl(newUrl)
			_uiState.update { currentState -> currentState.copy(revertUrl = newUrl) }
		}
	}

	fun revertUrl()
	{
		_uiState.update() { currentState -> currentState.copy(xtraUrl = currentState.revertUrl)}
	}

	private suspend fun downloadXtra(): Boolean
	{
		return withContext(Dispatchers.IO)
		{
			val url = _uiState.value.xtraUrl
			val fileName = "xtra.bin"
			val target = File(getApplication<Application>().filesDir, fileName)
			appendDebug("(1) Download xtra from ${url}")

			try
			{
				val connection = URL(url).openConnection()
				connection.connect()
				connection.getInputStream().use() { input -> FileOutputStream(target).use() { output -> input.copyTo(output) } }
				return@withContext true
			}
			catch(e: IOException)
			{
				appendDebug(e.stackTraceToString())
				return@withContext false
			}
		}
	}
}