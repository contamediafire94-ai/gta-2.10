package com.samp.mobile.launcher.activity;

import android.os.Bundle;
import android.content.Intent;
import android.content.SharedPreferences;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

public class MainActivity extends AppCompatActivity {

    private EditText editNick;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        editNick = findViewById(R.id.edit_nick);
        Button jogar = findViewById(R.id.button_play);

        prefs = getSharedPreferences("dz6_config", MODE_PRIVATE);

        String nickSalvo = prefs.getString("nickname", "");
        editNick.setText(nickSalvo);

        jogar.setOnClickListener(v -> {

            String nick = editNick.getText().toString().trim();

            if (nick.isEmpty()) {
                editNick.setError("Digite seu Nome_Sobrenome");
                return;
            }

            prefs.edit().putString("nickname", nick).apply();

            Intent intent = new Intent(MainActivity.this, SAMP.class);
intent.putExtra("nickname", nick);
startActivity(intent););
        });
    }
}
