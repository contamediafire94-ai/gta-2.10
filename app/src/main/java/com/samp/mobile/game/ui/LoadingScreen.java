package com.samp.mobile.game.ui;

import android.app.Activity;
import android.view.View;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.samp.mobile.R;

import java.util.Formatter;

public class LoadingScreen {

    private final Activity activity;
    private TextView percentText;
    private final ConstraintLayout mainLayout;

    private Runnable onUiReadyForNetwork;
    private boolean uiReadyDispatched = false;

    public LoadingScreen(Activity activity, Runnable onUiReadyForNetwork)
    {
        this.activity = activity;
        this.onUiReadyForNetwork = onUiReadyForNetwork;

        mainLayout = (ConstraintLayout) activity.getLayoutInflater()
                .inflate(R.layout.loadingscreen, null);

        activity.addContentView(
                mainLayout,
                new ConstraintLayout.LayoutParams(-1, -1)
        );

        /*
         * V15C:
         * Esta tela nao sabe quando o motor GTA terminou o loading.
         * O que ela sabe com seguranca e quando o overlay Java ja foi
         * anexado e teve oportunidade de entrar no pipeline da UI.
         *
         * A APK de referencia tambem separa "network init" do hide final
         * do loading. Entao liberamos a rede quando a UI Java esta pronta,
         * e deixamos o main.cpp continuar exigindo bGameInited antes de
         * realmente criar CNetGame.
         */
        mainLayout.post(new Runnable() {
            @Override
            public void run() {
                mainLayout.postOnAnimation(new Runnable() {
                    @Override
                    public void run() {
                        dispatchUiReadyForNetwork();
                    }
                });
            }
        });
    }

    private void dispatchUiReadyForNetwork()
    {
        if (uiReadyDispatched)
            return;

        uiReadyDispatched = true;

        if (onUiReadyForNetwork != null)
        {
            onUiReadyForNetwork.run();
            onUiReadyForNetwork = null;
        }
    }

    public void hide()
    {
        mainLayout.setVisibility(View.INVISIBLE);
    }

    public void show()
    {
        mainLayout.setVisibility(View.VISIBLE);
    }

    public void Update(int percent)
    {
        // O TextView de porcentagem nao era inicializado no arquivo original.
        // Mantemos o update seguro caso esse callback seja usado no futuro.
        if (percentText != null && percent >= 0 && percent <= 100)
        {
            percentText.setText(
                    new Formatter().format("%d%s", Integer.valueOf(percent), "%").toString()
            );
        }
    }
}
