// ILibLocAPI2Service.aidl
package dt.gpsxtra;
import dt.gpsxtra.ILibLocAPI2Callback;

// Declare any non-default types here with import statements

interface ILibLocAPI2Service
{
    void registerCallback(ILibLocAPI2Callback model);
    void removeCallback(ILibLocAPI2Callback model);
    void injectWrapper(String path);
}