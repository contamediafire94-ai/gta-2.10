package com.samp.mobile.game.ui;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.samp.mobile.R;

public class LoadingScreen {

    /*
     * Mantemos a tela WIU visível por um tempo mínimo para cobrir
     * completamente o splash antigo do cliente/GTA durante a transição.
     */
    private static final long MIN_VISIBLE_MS = 4500L;
    private static final long FINAL_DELAY_MS = 1000L;
    private static final long FADE_MS = 350L;

    private final Activity activity;
    private final ConstraintLayout mainLayout;
    private final Handler handler = new Handler(Looper.getMainLooper());

    private final TextView statusText;
    private final TextView serverText;

    private Runnable onUiReadyForNetwork;
    private boolean uiReadyDispatched = false;
    private boolean finishScheduled = false;
    private long shownAt;

    public LoadingScreen(Activity activity, Runnable onUiReadyForNetwork) {
        this.activity = activity;
        this.onUiReadyForNetwork = onUiReadyForNetwork;

        mainLayout = (ConstraintLayout) activity.getLayoutInflater()
                .inflate(R.layout.loadingscreen, null);

        statusText = mainLayout.findViewById(R.id.loading_status);
        serverText = mainLayout.findViewById(R.id.loading_server);

        activity.addContentView(
                mainLayout,
                new ConstraintLayout.LayoutParams(-1, -1)
        );

        shownAt = SystemClock.uptimeMillis();

        mainLayout.setAlpha(1.0f);
        mainLayout.setVisibility(View.VISIBLE);

        setStatus("Preparando o SA-MP Mobile...");

        /*
         * A rede continua sendo liberada assim que a UI Java está pronta.
         * Isso NÃO esconde a tela WIU.
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

    private void dispatchUiReadyForNetwork() {
        if (uiReadyDispatched) {
            return;
        }

        uiReadyDispatched = true;

        if (onUiReadyForNetwork != null) {
            onUiReadyForNetwork.run();
            onUiReadyForNetwork = null;
        }
    }

    public void setStatus(final String text) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (statusText != null && text != null) {
                    statusText.setText(text);
                }
            }
        });
    }

    public void setServerAddress(final String address) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (serverText == null) {
                    return;
                }

                if (address == null || address.trim().isEmpty()) {
                    serverText.setVisibility(View.GONE);
                } else {
                    serverText.setText(address.trim());
                    serverText.setVisibility(View.VISIBLE);
                }
            }
        });
    }

    /*
     * O native pode pedir para esconder a tela cedo.
     * Em vez de sumir imediatamente, seguramos a WIU Loading até a fase
     * de entrada estar madura o suficiente para evitar mostrar o splash antigo.
     */
    public void finishWhenReady(final String finalStatus) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (finishScheduled) {
                    return;
                }

                finishScheduled = true;

                if (finalStatus != null && statusText != null) {
                    statusText.setText(finalStatus);
                }

                long elapsed = SystemClock.uptimeMillis() - shownAt;
                long remainingMinimum = Math.max(0L, MIN_VISIBLE_MS - elapsed);
                long delay = Math.max(FINAL_DELAY_MS, remainingMinimum);

                handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mainLayout.animate()
                                .alpha(0.0f)
                                .setDuration(FADE_MS)
                                .withEndAction(new Runnable() {
                                    @Override
                                    public void run() {
                                        mainLayout.setVisibility(View.GONE);
                                    }
                                })
                                .start();
                    }
                }, delay);
            }
        });
    }

    public void hide() {
        finishWhenReady("Entrando no servidor...");
    }

    public void show() {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                handler.removeCallbacksAndMessages(null);

                finishScheduled = false;
                shownAt = SystemClock.uptimeMillis();

                mainLayout.animate().cancel();
                mainLayout.setAlpha(1.0f);
                mainLayout.setVisibility(View.VISIBLE);
            }
        });
    }

    public void Update(int percent) {
        if (percent >= 0 && percent <= 100) {
            setStatus("Carregando... " + percent + "%");
        }
    }
}
