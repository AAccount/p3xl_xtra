package dt.gpsxtra.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dt.gpsxtra.DEFAULT_XTRA_URL
import dt.gpsxtra.PreferencesDataStore
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

class MainModel(private val prefsKv: PreferencesDataStore) : ViewModel()
{
	data class MainState(
		val xtraUrl: String = DEFAULT_XTRA_URL,
		val lastRun: String = "2026-09-21 18:41:44.000", // when the android studio project was created
		val debugText: String = "",
		val isRunning: Boolean = false,
	)

	init
	{
		viewModelScope.launch {
			combine(prefsKv.xtraUrlFlow(), prefsKv.lastRunFlow()){ (xtraUrl, lastRun) -> Pair(xtraUrl, lastRun)}
				.collect() { (xtraUrl, lastRun) -> _uiState.update { currentState -> currentState.copy(xtraUrl = xtraUrl, lastRun = lastRun) } }
		}
	}

	private val _uiState = MutableStateFlow(MainState())
	val uiState = _uiState.asStateFlow()

	fun appendDebug(newText: String)
	{
		_uiState.update() { currentState -> currentState.copy(debugText = "${currentState.debugText}${newText}\n") }
	}

	fun setRunning(isRunning: Boolean)
	{
		_uiState.update() { currentState -> currentState.copy(isRunning = isRunning)}
	}

	fun updateUrl(newUrl: String)
	{
		_uiState.update() { currentState -> currentState.copy(xtraUrl = newUrl)}
	}
}