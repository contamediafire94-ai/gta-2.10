package com.samp.mobile.launcher.activity;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.provider.DocumentsContract;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_TEXDB_FOLDER = 9001;
    private static final int REQUEST_DATA_FOLDER = 9002;
    private static final int REQUEST_AUDIO_FOLDER = 9003;

    private EditText editNick;
    private SharedPreferences prefs;
    private ProgressDialog importDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        enableImmersiveMode();

        setContentView(R.layout.activity_main);

        editNick = findViewById(R.id.edit_nick);
        Button jogar = findViewById(R.id.button_play);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        String nickSalvo = prefs.getString("nickname", "");
        editNick.setText(nickSalvo);

        // MantÃ©m compatibilidade com quem jÃ¡ importou o texdb na build anterior.
        // Se texdb jÃ¡ estiver pronto, pede apenas a pasta data.
        if (!prefs.getBoolean("texdb_imported", false)) {
            Toast.makeText(
                    this,
                    "Selecione a pasta Download/BetaTesterData/texdb",
                    Toast.LENGTH_LONG
            ).show();

            findViewById(android.R.id.content).postDelayed(
                    this::openTexdbFolderPicker,
                    700
            );
        } else if (!prefs.getBoolean("data_imported", false)) {
            Toast.makeText(
                    this,
                    "Selecione a pasta Download/BetaTesterData/data",
                    Toast.LENGTH_LONG
            ).show();

            findViewById(android.R.id.content).postDelayed(
                    this::openDataFolderPicker,
                    700
            );
        } else if (!prefs.getBoolean("audio_imported", false)) {
            Toast.makeText(
                    this,
                    "Selecione a pasta Download/BetaTesterData/audio",
                    Toast.LENGTH_LONG
            ).show();

            findViewById(android.R.id.content).postDelayed(
                    this::openAudioFolderPicker,
                    700
            );
        }

        jogar.setOnClickListener(v -> {

            String nick = editNick.getText().toString().trim();

            if (nick.isEmpty()) {
                editNick.setError("Digite seu Nome_Sobrenome");
                return;
            }

            if (!prefs.getBoolean("texdb_imported", false)) {
                Toast.makeText(
                        MainActivity.this,
                        "Importe a pasta texdb primeiro.",
                        Toast.LENGTH_LONG
                ).show();
                openTexdbFolderPicker();
                return;
            }

            if (!prefs.getBoolean("data_imported", false)) {
                Toast.makeText(
                        MainActivity.this,
                        "Importe a pasta data primeiro.",
                        Toast.LENGTH_LONG
                ).show();
                openDataFolderPicker();
                return;
            }

            if (!prefs.getBoolean("audio_imported", false)) {
                Toast.makeText(
                        MainActivity.this,
                        "Importe a pasta audio primeiro.",
                        Toast.LENGTH_LONG
                ).show();
                openAudioFolderPicker();
                return;
            }

            prefs.edit().putString("nickname", nick).apply();

            Intent intent = new Intent(MainActivity.this, SAMP.class);
            intent.putExtra("nickname", nick);
            startActivity(intent);
        });
    }

    private void enableImmersiveMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(params);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().setDecorFitsSystemWindows(false);

            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(
                        WindowInsets.Type.statusBars()
                                | WindowInsets.Type.navigationBars()
                );

                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                );
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            );
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        enableImmersiveMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            enableImmersiveMode();
        }
    }

    private void openTexdbFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );
        startActivityForResult(intent, REQUEST_TEXDB_FOLDER);
    }

    private void openDataFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );
        startActivityForResult(intent, REQUEST_DATA_FOLDER);
    }

    private void openAudioFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );
        startActivityForResult(intent, REQUEST_AUDIO_FOLDER);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if ((requestCode != REQUEST_TEXDB_FOLDER
                && requestCode != REQUEST_DATA_FOLDER
                && requestCode != REQUEST_AUDIO_FOLDER)
                || resultCode != RESULT_OK
                || data == null
                || data.getData() == null) {
            return;
        }

        Uri treeUri = data.getData();

        try {
            getContentResolver().takePersistableUriPermission(
                    treeUri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
            );
        } catch (SecurityException ignored) {
        }

        final boolean importingTexdb = requestCode == REQUEST_TEXDB_FOLDER;
        final boolean importingData = requestCode == REQUEST_DATA_FOLDER;
        final boolean importingAudio = requestCode == REQUEST_AUDIO_FOLDER;

        final String destinationFolder =
                importingTexdb ? "texdb_app" :
                importingData ? "data_app" :
                "audio_app";

        final String prefKey =
                importingTexdb ? "texdb_imported" :
                importingData ? "data_imported" :
                "audio_imported";

        final String title =
                importingTexdb ? "Importando texdb" :
                importingData ? "Importando data" :
                "Importando audio";

        importDialog = new ProgressDialog(this);
        importDialog.setTitle(title);
        importDialog.setMessage("Copiando arquivos...\nIsso pode levar alguns minutos.");
        importDialog.setIndeterminate(true);
        importDialog.setCancelable(false);
        importDialog.show();

        new Thread(() -> {
            try {
                File root = getExternalFilesDir(null);

                if (root == null) {
                    throw new IOException("Pasta privada externa indisponÃ­vel.");
                }

                File destination = new File(root, destinationFolder);

                if (!destination.exists() && !destination.mkdirs()) {
                    throw new IOException(
                            "NÃ£o foi possÃ­vel criar: " + destination.getAbsolutePath()
                    );
                }

                String rootDocumentId = DocumentsContract.getTreeDocumentId(treeUri);
                Uri rootDocumentUri = DocumentsContract.buildDocumentUriUsingTree(
                        treeUri,
                        rootDocumentId
                );

                copyDirectoryContents(treeUri, rootDocumentUri, destination);

                prefs.edit().putBoolean(prefKey, true).apply();

                runOnUiThread(() -> {
                    if (importDialog != null && importDialog.isShowing()) {
                        importDialog.dismiss();
                    }

                    if (importingTexdb) {
                        Toast.makeText(
                                MainActivity.this,
                                "texdb importado. Agora selecione Download/BetaTesterData/data.",
                                Toast.LENGTH_LONG
                        ).show();

                        openDataFolderPicker();

                    } else if (importingData) {
                        Toast.makeText(
                                MainActivity.this,
                                "data importado. Agora selecione Download/BetaTesterData/audio.",
                                Toast.LENGTH_LONG
                        ).show();

                        openAudioFolderPicker();

                    } else if (importingAudio) {
                        Toast.makeText(
                                MainActivity.this,
                                "audio importado com sucesso. Agora pode tocar em JOGAR.",
                                Toast.LENGTH_LONG
                        ).show();
                    }
                });

            } catch (Exception e) {
                runOnUiThread(() -> {
                    if (importDialog != null && importDialog.isShowing()) {
                        importDialog.dismiss();
                    }

                    Toast.makeText(
                            MainActivity.this,
                            "Erro ao importar: " + e.getMessage(),
                            Toast.LENGTH_LONG
                    ).show();
                });
            }
        }).start();
    }

    private void copyDirectoryContents(
            Uri treeUri,
            Uri sourceDirectoryUri,
            File destinationDirectory
    ) throws IOException {

        ContentResolver resolver = getContentResolver();

        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(
                treeUri,
                DocumentsContract.getDocumentId(sourceDirectoryUri)
        );

        String[] projection = new String[]{
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE
        };

        try (Cursor cursor = resolver.query(
                childrenUri,
                projection,
                null,
                null,
                null
        )) {
            if (cursor == null) {
                throw new IOException("NÃ£o foi possÃ­vel listar a pasta selecionada.");
            }

            int idColumn = cursor.getColumnIndexOrThrow(
                    DocumentsContract.Document.COLUMN_DOCUMENT_ID
            );
            int nameColumn = cursor.getColumnIndexOrThrow(
                    DocumentsContract.Document.COLUMN_DISPLAY_NAME
            );
            int mimeColumn = cursor.getColumnIndexOrThrow(
                    DocumentsContract.Document.COLUMN_MIME_TYPE
            );

            while (cursor.moveToNext()) {
                String documentId = cursor.getString(idColumn);
                String displayName = cursor.getString(nameColumn);
                String mimeType = cursor.getString(mimeColumn);

                Uri childUri = DocumentsContract.buildDocumentUriUsingTree(
                        treeUri,
                        documentId
                );

                File output = new File(destinationDirectory, displayName);

                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType)) {
                    if (!output.exists() && !output.mkdirs()) {
                        throw new IOException(
                                "NÃ£o foi possÃ­vel criar a pasta: " + output.getAbsolutePath()
                        );
                    }

                    copyDirectoryContents(treeUri, childUri, output);

                } else {
                    copyFile(resolver, childUri, output);
                }
            }
        }
    }

    private void copyFile(
            ContentResolver resolver,
            Uri sourceUri,
            File destination
    ) throws IOException {

        try (InputStream input = resolver.openInputStream(sourceUri);
             FileOutputStream output = new FileOutputStream(destination, false)) {

            if (input == null) {
                throw new IOException("NÃ£o foi possÃ­vel abrir: " + sourceUri);
            }

            byte[] buffer = new byte[1024 * 1024];
            int read;

            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }

            output.flush();
        }
    }
          }
