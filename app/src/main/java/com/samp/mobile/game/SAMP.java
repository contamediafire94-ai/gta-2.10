package com.samp.mobile.game;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;

import com.joom.paranoid.Obfuscate;
import com.samp.mobile.game.ui.AttachEdit;
import com.samp.mobile.game.ui.CustomKeyboard;
import com.samp.mobile.game.ui.LoadingScreen;
import com.samp.mobile.game.ui.dialog.DialogManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;

@Obfuscate
public class SAMP extends GTASA implements
        CustomKeyboard.InputListener,
        HeightProvider.HeightListener {

    private static final String TAG = "SAMP";
    private static SAMP instance;

    private CustomKeyboard mKeyboard;
    private DialogManager mDialog;
    private HeightProvider mHeightProvider;

    private AttachEdit mAttachEdit;
    private LoadingScreen mLoadingScreen;

    public native void sendDialogResponse(int i, int i2, int i3, byte[] str);

    private native void initializeSAMP();
    private native void setLauncherNickname(String nickname);
    private native void onInputEnd(byte[] str);
    public native void onEventBackPressed();

    public static SAMP getInstance() {
        return instance;
    }

    @Override
    protected void attachBaseContext(Context newBase) {
        super.attachBaseContext(newBase);

        // Prepara o arquivo de idioma antes do onCreate/GTASA iniciar.
        // Ele vem de app/src/main/assets/AMERICAN.GXT e é gravado
        // pelo próprio aplicativo na pasta externa privada do app.
        prepareAmericanGxt(newBase);
    }

    private void prepareAmericanGxt(Context context) {
        File root = context.getExternalFilesDir(null);

        if (root == null) {
            Log.e(TAG, "Nao foi possivel obter a pasta externa privada do app");
            return;
        }

        // Grava diretamente na raiz privada externa do app.
        // A pasta SAMP existente está retornando EACCES / Permission denied
        // para os arquivos que foram colocados nela manualmente.
        File target = new File(root, "AMERICAN_APP.GXT");

        try (InputStream input = context.getAssets().open("AMERICAN.GXT");
             FileOutputStream output = new FileOutputStream(target, false)) {

            byte[] buffer = new byte[8192];
            int read;

            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }

            output.flush();

            Log.i(TAG, "AMERICAN.GXT preparado pelo app em: "
                    + target.getAbsolutePath()
                    + " | bytes=" + target.length());

        } catch (IOException e) {
            Log.e(TAG, "Erro ao preparar AMERICAN.GXT", e);
        }
    }

    @Override
    public void hideSystemUI() {
        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }

    private void showTab() {
    }

    private void hideTab() {
    }

    private void setTab(int id, String name, int score, int ping) {
    }

    private void clearTab() {
    }

    private void showLoadingScreen() {
    }

    private void hideLoadingScreen() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null) {
                    mLoadingScreen.hide();
                }
            }
        });
    }

    public void setPauseState(final boolean pause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (pause) {
                    if (mDialog != null) {
                        mDialog.hideWithoutReset();
                    }

                    if (mAttachEdit != null) {
                        mAttachEdit.hideWithoutReset();
                    }
                } else {
                    if (mDialog != null && mDialog.isShow) {
                        mDialog.showWithOldContent();
                    }

                    if (mAttachEdit != null && mAttachEdit.isShow) {
                        mAttachEdit.showWithoutReset();
                    }
                }
            }
        });
    }

    public void exitGame() {
        finishAndRemoveTask();
        System.exit(0);
    }

    public void showDialog(
            final int dialogId,
            final int dialogTypeId,
            byte[] bArr,
            byte[] bArr2,
            byte[] bArr3,
            byte[] bArr4
    ) {
        final String caption = new String(bArr, StandardCharsets.UTF_8);
        final String content = new String(bArr2, StandardCharsets.UTF_8);
        final String leftBtnText = new String(bArr3, StandardCharsets.UTF_8);
        final String rightBtnText = new String(bArr4, StandardCharsets.UTF_8);

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mDialog != null) {
                    mDialog.show(
                            dialogId,
                            dialogTypeId,
                            caption,
                            content,
                            leftBtnText,
                            rightBtnText
                    );
                }
            }
        });
    }

    @Override
    public void OnInputEnd(String str) {
        if (str == null) {
            return;
        }

        byte[] toReturn;

        try {
            toReturn = str.getBytes("windows-1251");
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "Erro ao converter input para windows-1251", e);
            return;
        }

        try {
            onInputEnd(toReturn);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Erro nativo em onInputEnd", e);
        }
    }

    private void showKeyboard() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "showKeyboard()");

                if (mKeyboard != null) {
                    mKeyboard.ShowInputLayout();
                }
            }
        });
    }

    private void hideKeyboard() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mKeyboard != null) {
                    mKeyboard.HideInputLayout();
                }
            }
        });
    }

    private void showEditObject() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAttachEdit != null) {
                    mAttachEdit.show();
                }
            }
        });
    }

    private void hideEditObject() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAttachEdit != null) {
                    mAttachEdit.hide();
                }
            }
        });
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "**** onCreate");

        super.onCreate(savedInstanceState);

        hideSystemUI();

        String nickname = getIntent().getStringExtra("nickname");

        mKeyboard = new CustomKeyboard(this);
        mDialog = new DialogManager(this);
        mAttachEdit = new AttachEdit(this);
        mLoadingScreen = new LoadingScreen(this);

        instance = this;

        try {
            if (nickname != null) {
                nickname = nickname.trim();

                if (!nickname.isEmpty()) {
                    Log.i(TAG, "Nickname recebido do launcher: " + nickname);
                    setLauncherNickname(nickname);
                }
            }

            initializeSAMP();

        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Erro ao inicializar biblioteca nativa do SA-MP", e);
        }
    }

    @Override
    public void onStart() {
        Log.i(TAG, "**** onStart");
        super.onStart();
        hideSystemUI();
    }

    @Override
    public void onRestart() {
        Log.i(TAG, "**** onRestart");
        super.onRestart();
        hideSystemUI();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "**** onResume");
        super.onResume();
        hideSystemUI();
        // mHeightProvider.init(view);
    }

    @Override
    public void onPause() {
        Log.i(TAG, "**** onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "**** onStop");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "**** onDestroy");
        instance = null;
        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            hideSystemUI();
        }
    }

    @Override
    public void onBackPressed() {
        try {
            onEventBackPressed();
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Erro nativo em onEventBackPressed", e);
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            try {
                onEventBackPressed();
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "Erro ao pressionar botão voltar", e);
            }

            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onHeightChanged(int orientation, int height) {
        // mKeyboard.onHeightChanged(height);
        // mDialog.onHeightChanged(height);
    }
}
