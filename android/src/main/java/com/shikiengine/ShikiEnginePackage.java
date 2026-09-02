package com.shikiengine;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.facebook.react.TurboReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfoProvider;

import java.util.Collections;

public class ShikiEnginePackage extends TurboReactPackage {
    static {
        try {
            System.loadLibrary("react-native-shiki-engine");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    @Override
    @Nullable
    public NativeModule getModule(@NonNull String name, @NonNull ReactApplicationContext reactContext) {
        return null;
    }

    @Override
    @NonNull
    public ReactModuleInfoProvider getReactModuleInfoProvider() {
        return Collections::emptyMap;
    }
}
