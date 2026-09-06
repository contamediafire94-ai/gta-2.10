package com.samp.mobile.launcher.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

public class ServersActivity extends AppCompatActivity {

    private EditText editServer;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_servers);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        editServer = findViewById(R.id.edit_server);
        Button jogar = findViewById(R.id.button_play);

        Button servidores = findViewById(R.id.button_servers);
        Button favoritos = findViewById(R.id.button_favorites);
        Button configuracoes = findViewById(R.id.button_settings);

        String servidorSalvo = prefs.getString(
                "server_address",
                "179.198.105.167:7125"
        );

        editServer.setText(servidorSalvo);

        jogar.setOnClickListener(v -> {
            String nick = prefs.getString("nickname", "").trim();
            String servidor = editServer.getText().toString().trim();

            if (nick.isEmpty()) {
                Toast.makeText(
                        ServersActivity.this,
                        "Nickname não encontrado. Volte e informe seu nick.",
                        Toast.LENGTH_LONG
                ).show();
                return;
            }

            if (servidor.isEmpty()) {
                editServer.setError("Digite IP:Porta");
                return;
            }

            prefs.edit()
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
}
