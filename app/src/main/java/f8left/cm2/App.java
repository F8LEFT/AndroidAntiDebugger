package f8left.cm2;

import android.app.Application;
import android.content.Context;
import android.os.Debug;
import android.os.Process;

import java.io.File;

public class App extends Application {
    static {
        System.loadLibrary("check");
    }

    protected void attachBaseContext(Context arg1) {
        super.attachBaseContext(arg1);
    }

    public void onCreate() {
        super.onCreate();
        if(checkDebugger() != 0) {
            ;
        }

    }

    private native int checkDebugger();
}
