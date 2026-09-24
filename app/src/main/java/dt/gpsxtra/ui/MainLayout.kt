package dt.gpsxtra.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
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
import dt.gpsxtra.ui.theme.GPSXtraTheme
import androidx.compose.runtime.collectAsState

val PADDING = 8.dp

@Composable
fun MainLayout(model: MainModel = viewModel())
{
	Column(
		modifier = Modifier.padding(PADDING)
	)
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
				onValueChange = { newValue -> model.updateUrl(newValue) } // TODO: need to update the model to send the change back?
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
		Text(
			text = stringResource(R.string.main_last_run) + " ${modelState.value.lastRun}",
			modifier = Modifier.padding(top = PADDING)
		)
		Button(
			modifier = Modifier.fillMaxWidth(),
			enabled = !modelState.value.isRunning,
			onClick = { model.injectXtra() }
		)
		{
			Text(text = stringResource(R.string.main_run))
		}
		TextField(
			value = modelState.value.debugText,
			onValueChange = {},
			readOnly = true,
			singleLine = false,
			placeholder = { Text("sample placeholder")},
			modifier = Modifier.fillMaxWidth().weight(1f)
		)
	}
}

@Preview(showBackground = true)
@Composable
fun MainPreview()
{
	GPSXtraTheme()
	{
		MainLayout(
//			xtraUrl = "https://xtrapath2.izatcloud.net/xtra3grcej.bin",
//			lastRan = "2026-09-21 18:41:44"
		)
	}
}