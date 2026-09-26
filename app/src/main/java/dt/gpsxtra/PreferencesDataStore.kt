package dt.gpsxtra

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.longPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter

val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "kv_store")
const val DEFAULT_XTRA_URL = "https://xtrapath2.izatcloud.net/xtra3grcej.bin"

class PreferencesDataStore(private val context: Context)
{
	companion object
	{
		val KEY_XTRA_URL = stringPreferencesKey("xtra-url")
		val KEY_LAST_RUN = longPreferencesKey("last-run")
	}

	fun xtraUrlFlow(): Flow<String>
	{
		return context.dataStore.data.map() { preferences -> preferences[KEY_XTRA_URL] ?: DEFAULT_XTRA_URL }
	}

	suspend fun saveXtraUrl(newUrl: String)
	{
		context.dataStore.updateData() { currentPrefs -> currentPrefs.toMutablePreferences().also() { prefs -> prefs[KEY_XTRA_URL] = newUrl } }
	}

	fun lastRunFlow(): Flow<String>
	{
		return context.dataStore.data.map { prefs ->
			val ts = Instant.ofEpochMilli(prefs[KEY_LAST_RUN] ?:  1789953812000) // when this function was fast written
			val formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS").withZone(ZoneId.systemDefault())
			formatter.format(ts)
		}
	}

	suspend fun saveLastRun(instant: Instant)
	{
		context.dataStore.updateData { currentPrefs -> currentPrefs.toMutablePreferences().also { prefs -> prefs[KEY_LAST_RUN] = instant.toEpochMilli()} }
	}
}