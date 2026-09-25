package dt.gpsxtra.activity

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import dt.gpsxtra.ui.MainLayout
import dt.gpsxtra.ui.theme.GPSXtraTheme

// New android studio project created: 2026-09-21 18:41:44
class MainActivity : ComponentActivity()
{
	override fun onCreate(savedInstanceState: Bundle?)
	{
	  super.onCreate(savedInstanceState)
		  enableEdgeToEdge() // seems to make the top status bar go away if you don't have it
		  setContent()
			{
				GPSXtraTheme()
				{
					Surface(
						modifier = Modifier.fillMaxSize()
							.safeDrawingPadding(), // prevents text from going into the notch
						color = MaterialTheme.colorScheme.background
					)
					{
						MainLayout(
//							xtraUrl = "https://xtrapath2.izatcloud.net/xtra3grcej.bin",
//							lastRan = "2026-09-21 18:41:44",
//							modifier = Modifier.padding(8.dp)
						)
					}
				}
		  }
	 }
}