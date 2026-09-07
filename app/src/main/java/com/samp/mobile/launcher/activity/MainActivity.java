package com.samp.mobile.launcher.activity;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

/**
 * Compatibilidade com builds antigas.
 *
 * A tela inicial oficial agora é ServersActivity pelo AndroidManifest.
 * Se alguma parte antiga ainda abrir MainActivity manualmente,
 * ela simplesmente redireciona para o launcher principal.
 */
public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = new Intent(MainActivity.this, ServersActivity.class);
        startActivity(intent);
        finish();
    }
}
