# Methods called from the dashboard's JavaScript (window.WTMSApp) must keep their names
-keepclassmembers class com.woodyouloveit.wtms.AppBridge {
    @android.webkit.JavascriptInterface <methods>;
}
-keepattributes JavascriptInterface
