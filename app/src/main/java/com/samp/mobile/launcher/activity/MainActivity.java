package com.samp.mobile.launcher.activity;

import android.os.Bundle;
import android.content.Intent;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        Button jogar = findViewById(R.id.button_play);

        jogar.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, SAMP.class);
            startActivity(intent);
        });
    }
}
