package dt.gpsxtra.ui

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.IconButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MaterialTheme.shapes
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import dt.gpsxtra.R
//import dt.gpsxtra.ui.theme.GPSXtraTheme
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.platform.LocalContext
import dt.gpsxtra.formatTimestamp
import java.time.Instant

val PADDING = 8.dp

@Composable
fun MainLayout(model: MainModel = viewModel())
{
	Column(
		modifier = Modifier.padding(PADDING)
	)
	{
		val modelState = model.uiState.collectAsState()
		XtraUrlSetup(model)
		RunButton(model)
		RunInformation(model)
		TextField(
			value = modelState.value.debugText,
			onValueChange = {},
			readOnly = true,
			singleLine = false,
			placeholder = { Text(stringResource(R.string.main_debug_placeholder))},
			modifier = Modifier.fillMaxWidth().weight(1f)
		)
	}
}

@Composable
fun RunInformation(model: MainModel)
{
	val modelState = model.uiState.collectAsState()
	val hasDebugText = modelState.value.debugText.isNotEmpty()
	val ctx = LocalContext.current
	val saveFileLauncher = rememberLauncherForActivityResult(contract = ActivityResultContracts.CreateDocument("text/plain"))
	{
		uri: Uri? -> uri?.let() { saveFile -> model.saveDebugOutput(ctx.contentResolver, saveFile) }
	}

	Row(verticalAlignment = Alignment.CenterVertically)
	{
		IconButton(
			onClick = { saveFileLauncher.launch("p3xl-xtra-${formatTimestamp(Instant.now())}.txt") },
			enabled = hasDebugText,
		)
		{
			Icon(
				painter = painterResource(R.drawable.download),
				tint = if(hasDebugText) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.primary.copy(alpha = 0.38f),
				contentDescription = stringResource(R.string.main_desc_save)
			)
		}
		Text(
			text = stringResource(R.string.main_last_run) + " ${modelState.value.lastRun}",
			modifier = Modifier.padding(top = PADDING).weight(1f)
		)
		IconButton(onClick = { model.clearDebugOutput() }, enabled = modelState.value.debugText.isNotEmpty())
		{
			Icon(
				painter = painterResource(R.drawable.delete),
				tint = if(hasDebugText) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.primary.copy(alpha = 0.38f),
				contentDescription = stringResource(R.string.main_clear_debug)
			)
		}
	}
}

@Composable
fun RunButton(model: MainModel)
{
	val modelState = model.uiState.collectAsState()
	Button(
		modifier = Modifier.fillMaxWidth(),
		enabled = !modelState.value.isRunning,
		onClick = { model.injectXtra() }
	)
	{
		Text(text = stringResource(R.string.main_run))
	}
}

@Composable
fun XtraUrlSetup(model: MainModel)
{
	val modelState = model.uiState.collectAsState()
	Row(verticalAlignment = Alignment.CenterVertically)
	{
		OutlinedTextField(
			value = modelState.value.xtraUrl,
			singleLine = true,
			shape = shapes.medium,
			modifier = Modifier.weight(1f),
			label = { Text(stringResource(R.string.main_xtra_url)) },
			onValueChange = { newValue -> model.updateUrl(newValue) }
		)
		IconButton(onClick = { model.revertUrl() })
		{
			Icon(
				painter = painterResource(R.drawable.undo),
				tint = MaterialTheme.colorScheme.primary,
				contentDescription = stringResource(R.string.main_desc_undo)
			)
		}
		IconButton(onClick = { model.saveUrl()})
		{
			Icon(
				painter = painterResource(R.drawable.save),
				tint = MaterialTheme.colorScheme.primary,
				contentDescription = stringResource(R.string.main_desc_save)
			)
		}
	}
}

@Preview(showBackground = true)
@Composable
fun MainPreview()
{

		MainLayout()
//			xtraUrl = "https://xtrapath2.izatcloud.net/xtra3grcej.bin",
//			lastRan = "2026-09-21 18:41:44"
//		)

}