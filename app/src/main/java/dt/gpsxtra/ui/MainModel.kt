package dt.gpsxtra.ui

import android.app.Application
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.os.RemoteException
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.topjohnwu.superuser.ipc.RootService
import dt.gpsxtra.DEFAULT_XTRA_URL
import dt.gpsxtra.ILibLocAPI2Callback
import dt.gpsxtra.ILibLocAPI2Service
import dt.gpsxtra.LibLocAPI2Service
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
	private val debugEvents = MutableSharedFlow<String>(extraBufferCapacity = 64) // need the buffer capacity or everything disappears
	private var xtraFilePath: String = ""
	private var libLocAPI2Service : ILibLocAPI2Service? = null

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

	fun updateUrl(newUrl: String)
	{
		_uiState.update() { currentState -> currentState.copy(xtraUrl = newUrl)}
	}

	fun saveUrl()
	{
		viewModelScope.launch()
		{
			val newUrl = _uiState.value.xtraUrl
			prefsKv.saveXtraUrl(newUrl)
			_uiState.update { currentState -> currentState.copy(revertUrl = newUrl) }
		}
	}

	fun revertUrl()
	{
		_uiState.update() { currentState -> currentState.copy(xtraUrl = currentState.revertUrl)}
	}

	fun injectXtra()
	{
		updateIsRunning(true)
		viewModelScope.launch()
		{
			val fileName = "xtra.bin"
			val target = File(getApplication<Application>().filesDir, fileName)
			xtraFilePath = target.absolutePath
			val downloadOk = downloadXtra(target)
			if(!downloadOk)
			{
				appendDebug("Failed to download xtra data")
				updateIsRunning(false)
				return@launch
			}

			appendDebug("Would run C code now")
			val ctx = getApplication<Application>()
			val intent = Intent(ctx, LibLocAPI2Service::class.java)
			RootService.bind(intent, libLocApi2Connection)
		}
	}

	private val serviceCallback = object : ILibLocAPI2Callback.Stub()
	{
		override fun onDebugMessage(debugMessage: String?)
		{
			if(debugMessage != null)
			{
				appendDebug(debugMessage)
			}
		}
	}

	private val libLocApi2Connection = object : ServiceConnection
	{
		override fun onServiceConnected(name: ComponentName?, service: IBinder?)
		{
			libLocAPI2Service = ILibLocAPI2Service.Stub.asInterface(service)
			try
			{
				libLocAPI2Service?.registerCallback(serviceCallback)
				libLocAPI2Service?.injectWrapper(xtraFilePath)
			}
			catch(e: RemoteException)
			{
				appendDebug(e.stackTraceToString())
			}
			updateIsRunning(false)
		}

		override fun onServiceDisconnected(name: ComponentName?)
		{
			try
			{
				libLocAPI2Service?.removeCallback(serviceCallback)
			}
			catch(e: RemoteException)
			{
				appendDebug(e.stackTraceToString())
			}
			libLocAPI2Service = null
		}

	}

	private fun updateIsRunning(isRunning: Boolean)
	{
		_uiState.update() { currentState -> currentState.copy(isRunning = isRunning) }
	}

	private suspend fun downloadXtra(target: File): Boolean
	{
		return withContext(Dispatchers.IO)
		{
			val url = _uiState.value.xtraUrl
			appendDebug("Download xtra from ${url} to ${target.absolutePath}")

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