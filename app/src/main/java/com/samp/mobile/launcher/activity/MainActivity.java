package com.samp.mobile.launcher.activity;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
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

    private static final int REQUEST_DZ6_DATA_FOLDER = 9010;
    private static final String PREF_DATA_READY = "dz6_data_imported";

    private EditText editNick;
    private EditText editServer;
    private SharedPreferences prefs;
    private ProgressDialog importDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editNick = findViewById(R.id.edit_nick);
        editServer = findViewById(R.id.edit_server);
        Button jogar = findViewById(R.id.button_play);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        String nickSalvo = prefs.getString("nickname", "");
        String servidorSalvo = prefs.getString("server_address", "179.198.105.167:7125");
        editNick.setText(nickSalvo);
        editServer.setText(servidorSalvo);

        // Compatibilidade com as builds anteriores, que importavam cada pasta separadamente.
        if (legacyDataIsReady() && !prefs.getBoolean(PREF_DATA_READY, false)) {
            prefs.edit().putBoolean(PREF_DATA_READY, true).apply();
        }

        if (!isDataReady()) {
            Toast.makeText(
                    this,
                    "Selecione a pasta DZ6Data completa.",
                    Toast.LENGTH_LONG
            ).show();

            findViewById(android.R.id.content).postDelayed(
                    this::openDz6DataFolderPicker,
                    700
            );
        }

        jogar.setOnClickListener(v -> {
            String nick = editNick.getText().toString().trim();
            String servidor = editServer.getText().toString().trim();

            if (nick.isEmpty()) {
                editNick.setError("Digite seu Nome_Sobrenome");
                return;
            }

            if (servidor.isEmpty()) {
                editServer.setError("Digite IP:Porta");
                return;
            }

            if (!isDataReady()) {
                Toast.makeText(
                        MainActivity.this,
                        "Instale a Data MOD DZ6 antes de jogar.",
                        Toast.LENGTH_LONG
                ).show();
                openDz6DataFolderPicker();
                return;
            }

            prefs.edit()
                    .putString("nickname", nick)
                    .putString("server_address", servidor)
                    .apply();

            Intent intent = new Intent(MainActivity.this, SAMP.class);
            intent.putExtra("nickname", nick);
            intent.putExtra("server_address", servidor);
            startActivity(intent);
        });
    }

    private boolean isDataReady() {
        return prefs.getBoolean(PREF_DATA_READY, false) || legacyDataIsReady();
    }

    private boolean legacyDataIsReady() {
        return prefs.getBoolean("texdb_imported", false)
                && prefs.getBoolean("data_imported", false)
                && prefs.getBoolean("audio_imported", false)
                && prefs.getBoolean("stream_imported", false)
                && prefs.getBoolean("samp_imported", false)
                && prefs.getBoolean("models_imported", false);
    }

    private void openDz6DataFolderPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
        );
        startActivityForResult(intent, REQUEST_DZ6_DATA_FOLDER);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode != REQUEST_DZ6_DATA_FOLDER
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
                            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
            );
        } catch (SecurityException ignored) {
            // A permissão temporária da Activity ainda permite concluir esta importação.
        }

        importDz6DataPack(treeUri);
    }

    private void importDz6DataPack(Uri treeUri) {
        importDialog = new ProgressDialog(this);
        importDialog.setTitle("Instalando Data MOD DZ6");
        importDialog.setMessage("Copiando arquivos...\nNão feche o launcher.");
        importDialog.setIndeterminate(true);
        importDialog.setCancelable(false);
        importDialog.show();

        new Thread(() -> {
            try {
                File root = getExternalFilesDir(null);
                if (root == null) {
                    throw new IOException("Pasta privada do jogo indisponível.");
                }

                String rootDocumentId = DocumentsContract.getTreeDocumentId(treeUri);
                Uri rootDocumentUri = DocumentsContract.buildDocumentUriUsingTree(
                        treeUri,
                        rootDocumentId
                );

                DocumentEntry texdb = findChild(treeUri, rootDocumentUri, "texdb");
                DocumentEntry data = findChild(treeUri, rootDocumentUri, "data");
                DocumentEntry audio = findChild(treeUri, rootDocumentUri, "audio");
                DocumentEntry samp = findChild(treeUri, rootDocumentUri, "SAMP");
                DocumentEntry models = findChild(treeUri, rootDocumentUri, "models");
                DocumentEntry stream = findChild(treeUri, rootDocumentUri, "stream.ini");

                requireDirectory(texdb, "texdb");
                requireDirectory(data, "data");
                requireDirectory(audio, "audio");
                requireDirectory(samp, "SAMP");
                requireDirectory(models, "models");
                requireFile(stream, "stream.ini");

                File texdbDest = prepareCleanDirectory(root, "texdb_app");
                File dataDest = prepareCleanDirectory(root, "data_app");
                File audioDest = prepareCleanDirectory(root, "audio_app");
                File sampDest = prepareCleanDirectory(root, "SAMP_app");
                File modelsDest = prepareCleanDirectory(root, "models");

                copyDirectoryContents(treeUri, texdb.uri, texdbDest);
                copyDirectoryContents(treeUri, data.uri, dataDest);
                copyDirectoryContents(treeUri, audio.uri, audioDest);
                copyDirectoryContents(treeUri, samp.uri, sampDest);
                copyDirectoryContents(treeUri, models.uri, modelsDest);

                File streamDest = new File(root, "stream_app.ini");
                copyFile(getContentResolver(), stream.uri, streamDest);

                if (!streamDest.isFile() || streamDest.length() <= 0) {
                    throw new IOException("stream_app.ini não foi criado corretamente.");
                }

                // Mantém as chaves antigas para não quebrar outras partes da source.
                prefs.edit()
                        .putBoolean(PREF_DATA_READY, true)
                        .putBoolean("texdb_imported", true)
                        .putBoolean("data_imported", true)
                        .putBoolean("audio_imported", true)
                        .putBoolean("stream_imported", true)
                        .putBoolean("samp_imported", true)
                        .putBoolean("models_imported", true)
                        .apply();

                runOnUiThread(() -> {
                    dismissImportDialog();
                    Toast.makeText(
                            MainActivity.this,
                            "Data MOD DZ6 instalada. Agora pode tocar em JOGAR.",
                            Toast.LENGTH_LONG
                    ).show();
                });

            } catch (Exception e) {
                prefs.edit().putBoolean(PREF_DATA_READY, false).apply();

                runOnUiThread(() -> {
                    dismissImportDialog();
                    Toast.makeText(
                            MainActivity.this,
                            "Erro ao instalar a Data MOD: " + e.getMessage(),
                            Toast.LENGTH_LONG
                    ).show();
                });
            }
        }).start();
    }

    private void dismissImportDialog() {
        if (importDialog != null && importDialog.isShowing()) {
            importDialog.dismiss();
        }
    }

    private File prepareCleanDirectory(File root, String folderName) throws IOException {
        File destination = new File(root, folderName);

        if (destination.exists() && !deleteRecursively(destination)) {
            throw new IOException("Não foi possível limpar: " + folderName);
        }

        if (!destination.mkdirs() && !destination.isDirectory()) {
            throw new IOException("Não foi possível criar: " + destination.getAbsolutePath());
        }

        return destination;
    }

    private boolean deleteRecursively(File file) {
        if (file == null || !file.exists()) {
            return true;
        }

        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    if (!deleteRecursively(child)) {
                        return false;
                    }
                }
            }
        }

        return file.delete();
    }

    private void requireDirectory(DocumentEntry entry, String name) throws IOException {
        if (entry == null || !entry.directory) {
            throw new IOException("Pasta obrigatória não encontrada: " + name);
        }
    }

    private void requireFile(DocumentEntry entry, String name) throws IOException {
        if (entry == null || entry.directory) {
            throw new IOException("Arquivo obrigatório não encontrado: " + name);
        }
    }

    private DocumentEntry findChild(Uri treeUri, Uri parentDirectoryUri, String wantedName)
            throws IOException {

        ContentResolver resolver = getContentResolver();
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(
                treeUri,
                DocumentsContract.getDocumentId(parentDirectoryUri)
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
                throw new IOException("Não foi possível ler a pasta DZ6Data.");
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
                String displayName = cursor.getString(nameColumn);
                if (displayName == null || !displayName.equalsIgnoreCase(wantedName)) {
                    continue;
                }

                String documentId = cursor.getString(idColumn);
                String mimeType = cursor.getString(mimeColumn);
                Uri childUri = DocumentsContract.buildDocumentUriUsingTree(
                        treeUri,
                        documentId
                );

                boolean directory = DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType);
                return new DocumentEntry(childUri, directory);
            }
        }

        return null;
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
                throw new IOException("Não foi possível listar a pasta selecionada.");
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
                                "Não foi possível criar a pasta: " + output.getAbsolutePath()
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

        File parent = destination.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Não foi possível criar: " + parent.getAbsolutePath());
        }

        try (InputStream input = resolver.openInputStream(sourceUri);
             FileOutputStream output = new FileOutputStream(destination, false)) {

            if (input == null) {
                throw new IOException("Não foi possível abrir: " + sourceUri);
            }

            byte[] buffer = new byte[1024 * 1024];
            int read;

            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }

            output.flush();
        }
    }

    private static class DocumentEntry {
        final Uri uri;
        final boolean directory;

        DocumentEntry(Uri uri, boolean directory) {
            this.uri = uri;
            this.directory = directory;
        }
    }
}
