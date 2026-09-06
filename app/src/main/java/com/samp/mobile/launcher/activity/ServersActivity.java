package com.samp.mobile.launcher.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

public class ServersActivity extends AppCompatActivity {

    private static final String TEST_SERVER_NAME = "GM Teste";
    private static final String TEST_SERVER_ADDRESS = "179.198.105.167:7125";

    private EditText editNick;
    private EditText editServer;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_servers);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        editNick = findViewById(R.id.edit_nick);
        editServer = findViewById(R.id.edit_server);

        Button jogar = findViewById(R.id.button_play);
        Button servidores = findViewById(R.id.button_servers);
        Button favoritos = findViewById(R.id.button_favorites);
        Button configuracoes = findViewById(R.id.button_settings);

        String nickSalvo = prefs.getString("nickname", "");
        String servidorSalvo = prefs.getString(
                "server_address",
                TEST_SERVER_ADDRESS
        );

        editNick.setText(nickSalvo);
        editServer.setText(servidorSalvo);

        criarCardServidor();

        jogar.setOnClickListener(v -> {
            String nick = editNick.getText().toString().trim();
            String servidor = editServer.getText().toString().trim();

            if (nick.isEmpty()) {
                editNick.setError("Digite seu Nome_Sobrenome");
                editNick.requestFocus();
                return;
            }

            if (servidor.isEmpty()) {
                editServer.setError("Digite IP:Porta");
                editServer.requestFocus();
                return;
            }

            prefs.edit()
                    .putString("nickname", nick)
                    .putString("server_address", servidor)
                    .apply();

            Intent intent = new Intent(ServersActivity.this, SAMP.class);
            intent.putExtra("nickname", nick);
            intent.putExtra("server_address", servidor);
            startActivity(intent);
        });

        servidores.setOnClickListener(v ->
                Toast.makeText(this, "Servidores", Toast.LENGTH_SHORT).show()
        );

        favoritos.setOnClickListener(v ->
                Toast.makeText(this, "Favoritos: próxima etapa", Toast.LENGTH_SHORT).show()
        );

        configuracoes.setOnClickListener(v ->
                Toast.makeText(this, "Configurações: próxima etapa", Toast.LENGTH_SHORT).show()
        );
    }

    private void criarCardServidor() {
        LinearLayout container = findViewById(R.id.server_list_container);

        // Mantém o título da área e remove apenas os textos de placeholder.
        while (container.getChildCount() > 1) {
            container.removeViewAt(1);
        }

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(18), dp(12), dp(16), dp(12));
        card.setBackgroundColor(Color.parseColor("#171C24"));
        card.setClickable(true);
        card.setFocusable(true);

        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(76)
        );
        cardParams.topMargin = dp(14);
        card.setLayoutParams(cardParams);

        LinearLayout info = new LinearLayout(this);
        info.setOrientation(LinearLayout.VERTICAL);

        LinearLayout.LayoutParams infoParams = new LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1f
        );
        info.setLayoutParams(infoParams);

        TextView nome = new TextView(this);
        nome.setText(TEST_SERVER_NAME);
        nome.setTextColor(Color.WHITE);
        nome.setTextSize(17);
        nome.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        TextView endereco = new TextView(this);
        endereco.setText(TEST_SERVER_ADDRESS);
        endereco.setTextColor(Color.parseColor("#818B9A"));
        endereco.setTextSize(13);

        TextView descricao = new TextView(this);
        descricao.setText("Servidor de teste • toque para selecionar");
        descricao.setTextColor(Color.parseColor("#5F6978"));
        descricao.setTextSize(11);

        info.addView(nome);
        info.addView(endereco);
        info.addView(descricao);

        TextView status = new TextView(this);
        status.setText("TESTE");
        status.setTextColor(Color.parseColor("#8DB5FF"));
        status.setTextSize(12);
        status.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        status.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                dp(90),
                dp(40)
        );
        status.setLayoutParams(statusParams);

        card.addView(info);
        card.addView(status);

        card.setOnClickListener(v -> {
            editServer.setText(TEST_SERVER_ADDRESS);

            Toast.makeText(
                    ServersActivity.this,
                    TEST_SERVER_NAME + " selecionado",
                    Toast.LENGTH_SHORT
            ).show();
        });

        container.addView(card);
    }

    private int dp(int value) {
        return Math.round(
                value * getResources().getDisplayMetrics().density
        );
    }
}
