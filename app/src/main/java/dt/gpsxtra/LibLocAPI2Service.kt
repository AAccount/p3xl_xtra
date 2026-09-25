package dt.gpsxtra

import android.content.Intent
import android.os.IBinder
import com.topjohnwu.superuser.ipc.RootService

class LibLocAPI2Service : RootService()
{
	companion object
	{
		init
		{
       System.loadLibrary("gpsxtra")
    }
	}

	private var mainModelCallback: ILibLocAPI2Callback? = null;
	private val binder = object : ILibLocAPI2Service.Stub()
	{
		override fun registerCallback(model: ILibLocAPI2Callback?)
		{
			mainModelCallback = model;
		}

		override fun removeCallback(model: ILibLocAPI2Callback?)
		{
			if(mainModelCallback == model)
			{
				mainModelCallback = null
			}
		}

		override fun injectWrapper(path: String?)
		{
			mainModelCallback?.onDebugMessage("Begin JNI")
			mainModelCallback?.onDebugMessage("Finished JNI")
		}
	}

	override fun onBind(intent: Intent): IBinder?
	{
		return binder;
	}
	@Suppress("unused") // it is used from the C side
	private fun debugFromC(message: String)
	{
		mainModelCallback?.onDebugMessage(message)
	}

	external fun inject(xtrbinPath: String)

}
