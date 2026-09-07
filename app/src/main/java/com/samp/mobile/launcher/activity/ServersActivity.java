package com.samp.mobile.launcher.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.samp.mobile.R;
import com.samp.mobile.game.SAMP;

import org.json.JSONArray;
import org.json.JSONObject;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class ServersActivity extends AppCompatActivity {

    private static final String TEST_SERVER_NAME = "GM Teste";
    private static final String TEST_SERVER_ADDRESS = "179.198.105.167:7125";

    private static final String PREF_CUSTOM_SERVERS = "custom_servers_json";
    private static final String PREF_FAVORITES = "favorite_servers";
    private static final String PREF_TEST_SERVER_ADDRESS = "test_server_address_override";
    private static final String PREF_TEST_SERVER_HIDDEN = "test_server_hidden";

    // Coloque aqui o convite oficial da sua comunidade quando quiser ativar o botão.
    private static final String DISCORD_URL = "";

    private SharedPreferences prefs;

    private TextView textTitle;
    private TextView textSubtitle;
    private TextView textLauncherUsers;
    private TextView textLauncherStatus;

    private LinearLayout serverListContainer;
    private LinearLayout featuredContainer;
    private LinearLayout favoritesPreviewContainer;
    private LinearLayout featuredBlock;

    private Button buttonServers;
    private Button buttonFavorites;
    private Button buttonDiscord;
    private Button buttonSettings;
    private EditText editNick;

    private int currentSection = 0;
    // 0 = servidores | 1 = favoritos | 2 = configurações

    private String selectedServerAddress = TEST_SERVER_ADDRESS;
    private String selectedServerName = TEST_SERVER_NAME;
    private Button buttonPlay;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        aplicarModoImersivo();
        setContentView(R.layout.activity_servers);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        textTitle = findViewById(R.id.text_title);
        textSubtitle = findViewById(R.id.text_subtitle);
        textLauncherUsers = findViewById(R.id.text_launcher_users);
        textLauncherStatus = findViewById(R.id.text_launcher_status);

        serverListContainer = findViewById(R.id.server_list_container);
        featuredContainer = findViewById(R.id.featured_container);
        favoritesPreviewContainer = findViewById(R.id.favorites_preview_container);
        featuredBlock = findViewById(R.id.featured_block);

        buttonServers = findViewById(R.id.button_servers);
        buttonFavorites = findViewById(R.id.button_favorites);
        buttonDiscord = findViewById(R.id.button_discord);
        buttonSettings = findViewById(R.id.button_settings);
        buttonPlay = findViewById(R.id.button_play);
        editNick = findViewById(R.id.edit_nick);

        selectedServerAddress = prefs.getString("server_address", "");

        List<ServerItem> iniciais = carregarTodosServidores();

        if (selectedServerAddress.trim().isEmpty()) {
            if (!iniciais.isEmpty()) {
                selectedServerAddress = iniciais.get(0).address;
                selectedServerName = iniciais.get(0).name;
            }
        } else {
            for (ServerItem servidor : iniciais) {
                if (servidor.address.equalsIgnoreCase(selectedServerAddress)) {
                    selectedServerName = servidor.name.isEmpty()
                            ? servidor.address
                            : servidor.name;
                    break;
                }
            }
        }

        editNick.setText(prefs.getString("nickname", ""));

        buttonPlay.setOnClickListener(v -> jogarServidorSelecionado());

        buttonServers.setOnClickListener(v -> mostrarListaServidores());
        buttonFavorites.setOnClickListener(v -> mostrarFavoritos());
        buttonDiscord.setOnClickListener(v -> abrirDiscord());
        buttonSettings.setOnClickListener(v -> mostrarConfiguracoes());

        prepararContadorLauncher();
        atualizarStatusLauncher();
        mostrarListaServidores();
    }


    @Override
    protected void onResume() {
        super.onResume();
        aplicarModoImersivo();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            aplicarModoImersivo();
        }
    }

    private void aplicarModoImersivo() {
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
            getWindow().setFlags(
                    WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN
            );

            View decor = getWindow().getDecorView();
            decor.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
        }
    }

    private void mostrarListaServidores() {
        currentSection = 0;

        textTitle.setText("Servidores");
        textSubtitle.setText("Escolha um servidor e entre direto pelo launcher");

        featuredBlock.setVisibility(View.VISIBLE);

        atualizarDestaqueMenu();
        limparLista();
        atualizarDestaques();
        atualizarFavoritosPreview();
        atualizarStatusLauncher();

        definirTituloLista("TODOS OS SERVIDORES");
        adicionarBotaoAdicionarServidor();

        for (ServerItem servidor : carregarTodosServidores()) {
            adicionarCardServidor(servidor);
        }
    }

    private void mostrarFavoritos() {
        currentSection = 1;

        textTitle.setText("Favoritos");
        textSubtitle.setText("Seus servidores salvos");

        featuredBlock.setVisibility(View.GONE);

        atualizarDestaqueMenu();
        limparLista();
        atualizarFavoritosPreview();
        atualizarStatusLauncher();
        definirTituloLista("MEUS FAVORITOS");

        Set<String> favoritos = carregarFavoritos();
        int encontrados = 0;

        for (ServerItem servidor : carregarTodosServidores()) {
            if (favoritos.contains(servidor.address)) {
                adicionarCardServidor(servidor);
                encontrados++;
            }
        }

        if (encontrados == 0) {
            TextView vazio = new TextView(this);
            vazio.setText(
                    "Você ainda não favoritou nenhum servidor.\n" +
                    "Volte em INÍCIO e toque na estrela ★."
            );
            vazio.setTextColor(Color.parseColor("#7F8998"));
            vazio.setTextSize(14);
            vazio.setPadding(0, dp(18), 0, 0);
            serverListContainer.addView(vazio);
        }
    }

    private void mostrarConfiguracoes() {
        currentSection = 2;

        textTitle.setText("Configurações");
        textSubtitle.setText("Ajustes do launcher");

        featuredBlock.setVisibility(View.GONE);

        atualizarDestaqueMenu();
        limparLista();
        atualizarFavoritosPreview();
        atualizarStatusLauncher();
        definirTituloLista("PERFIL DO JOGADOR");

        TextView labelNick = criarLabel("NICKNAME PADRÃO");
        serverListContainer.addView(labelNick);

        EditText editNick = new EditText(this);
        editNick.setHint("Nome_Sobrenome");
        editNick.setSingleLine(true);
        editNick.setText(prefs.getString("nickname", ""));
        editNick.setTextColor(Color.WHITE);
        editNick.setHintTextColor(Color.parseColor("#626B79"));
        editNick.setTextSize(15);
        editNick.setBackgroundTintList(
                ColorStateList.valueOf(Color.parseColor("#3C4553"))
        );
        editNick.setPadding(dp(10), 0, dp(10), 0);

        LinearLayout.LayoutParams nickParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(48)
        );
        nickParams.topMargin = dp(8);
        editNick.setLayoutParams(nickParams);

        serverListContainer.addView(editNick);

        Button salvarNick = criarBotaoSecundario("SALVAR NICK");
        salvarNick.setOnClickListener(v -> {
            String nick = editNick.getText().toString().trim();

            if (nick.isEmpty()) {
                editNick.setError("Digite seu Nome_Sobrenome");
                editNick.requestFocus();
                return;
            }

            prefs.edit()
                    .putString("nickname", nick)
                    .apply();

            editNick.setText(nick);

            Toast.makeText(
                    this,
                    "Nickname salvo",
                    Toast.LENGTH_SHORT
            ).show();
        });

        serverListContainer.addView(salvarNick);

        TextView labelDados = criarLabel("GERENCIAR LISTAS");
        LinearLayout.LayoutParams labelDadosParams =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT
                );
        labelDadosParams.topMargin = dp(22);
        labelDados.setLayoutParams(labelDadosParams);
        serverListContainer.addView(labelDados);

        Button limparFavoritos = criarBotaoSecundario("LIMPAR FAVORITOS");
        limparFavoritos.setOnClickListener(v -> new AlertDialog.Builder(this)
                .setTitle("Limpar favoritos")
                .setMessage("Remover todos os servidores dos favoritos?")
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("LIMPAR", (dialog, which) -> {
                    prefs.edit()
                            .remove(PREF_FAVORITES)
                            .apply();

                    Toast.makeText(
                            this,
                            "Favoritos limpos",
                            Toast.LENGTH_SHORT
                    ).show();
                })
                .show());

        serverListContainer.addView(limparFavoritos);

        Button limparAdicionados = criarBotaoSecundario(
                "REMOVER SERVIDORES ADICIONADOS"
        );
        limparAdicionados.setOnClickListener(v -> new AlertDialog.Builder(this)
                .setTitle("Remover servidores")
                .setMessage(
                        "Isso remove apenas os servidores adicionados por você."
                )
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("REMOVER", (dialog, which) -> {
                    prefs.edit()
                            .putString(PREF_CUSTOM_SERVERS, "[]")
                            .apply();

                    Toast.makeText(
                            this,
                            "Servidores adicionados removidos",
                            Toast.LENGTH_SHORT
                    ).show();
                })
                .show());

        serverListContainer.addView(limparAdicionados);

        TextView dica = new TextView(this);
        dica.setText(
                "Selecione um servidor na lista e depois toque em JOGAR."
        );
        dica.setTextColor(Color.parseColor("#697383"));
        dica.setTextSize(12);
        dica.setPadding(0, dp(18), 0, dp(4));
        serverListContainer.addView(dica);
    }

    private TextView criarLabel(String texto) {
        TextView label = new TextView(this);
        label.setText(texto);
        label.setTextColor(Color.parseColor("#737D8D"));
        label.setTextSize(11);
        label.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        label.setLetterSpacing(0.08f);
        return label;
    }

    private Button criarBotaoSecundario(String texto) {
        Button botao = new Button(this);
        botao.setText(texto);
        botao.setTextColor(Color.WHITE);
        botao.setTextSize(11);
        botao.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        botao.setAllCaps(false);
        botao.setBackgroundTintList(
                ColorStateList.valueOf(Color.parseColor("#202631"))
        );

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dp(42)
        );
        params.topMargin = dp(10);
        botao.setLayoutParams(params);

        return botao;
    }

    private void atualizarDestaqueMenu() {
        int ativo = Color.parseColor("#2D6CDF");
        int inativo = Color.parseColor("#181D25");

        buttonServers.setBackgroundTintList(
                ColorStateList.valueOf(currentSection == 0 ? ativo : inativo)
        );

        buttonFavorites.setBackgroundTintList(
                ColorStateList.valueOf(currentSection == 1 ? ativo : inativo)
        );

        buttonSettings.setBackgroundTintList(
                ColorStateList.valueOf(currentSection == 2 ? ativo : inativo)
        );

        if (buttonDiscord != null) {
            buttonDiscord.setBackgroundTintList(
                    ColorStateList.valueOf(inativo)
            );
        }
    }

    private void limparLista() {
        serverListContainer.removeAllViews();
    }

    private void definirTituloLista(String texto) {
        TextView titulo = criarLabel(texto);
        serverListContainer.addView(titulo);
    }

    private void adicionarBotaoAdicionarServidor() {
        Button adicionar = new Button(this);
        adicionar.setText("+  ADICIONAR SERVIDOR");
        adicionar.setTextColor(Color.WHITE);
        adicionar.setTextSize(11);
        adicionar.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        adicionar.setAllCaps(false);
        adicionar.setBackgroundTintList(
                ColorStateList.valueOf(Color.parseColor("#202631"))
        );

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                dp(220),
                dp(38)
        );
        params.topMargin = dp(10);
        adicionar.setLayoutParams(params);

        adicionar.setOnClickListener(v -> abrirDialogAdicionarServidor());

        serverListContainer.addView(adicionar);
    }

    private void abrirDialogAdicionarServidor() {
        LinearLayout conteudo = new LinearLayout(this);
        conteudo.setOrientation(LinearLayout.VERTICAL);
        conteudo.setPadding(dp(24), dp(8), dp(24), 0);

        EditText endereco = new EditText(this);
        endereco.setHint("IP:Porta  •  ex: 144.217.62.159:7777");
        endereco.setSingleLine(true);
        endereco.setInputType(
                InputType.TYPE_CLASS_TEXT
                        | InputType.TYPE_TEXT_VARIATION_URI
        );

        conteudo.addView(endereco);

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Adicionar servidor")
                .setMessage(
                        "Digite o endereço completo. O nome será detectado automaticamente."
                )
                .setView(conteudo)
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("SALVAR", null)
                .create();

        dialog.setOnShowListener(d -> dialog
                .getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String enderecoFinal = validarEnderecoServidor(
                            endereco,
                            endereco.getText().toString().trim()
                    );

                    if (enderecoFinal == null) {
                        return;
                    }

                    if (servidorJaExiste(enderecoFinal)) {
                        Toast.makeText(
                                this,
                                "Esse servidor já está na lista",
                                Toast.LENGTH_SHORT
                        ).show();
                        return;
                    }

                    salvarServidorPersonalizado(
                            new ServerItem("", enderecoFinal, true)
                    );

                    dialog.dismiss();

                    Toast.makeText(
                            this,
                            "Servidor adicionado",
                            Toast.LENGTH_SHORT
                    ).show();

                    mostrarListaServidores();
                }));

        dialog.show();
    }

    private void adicionarCardServidor(ServerItem servidor) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(12), dp(7), dp(4), dp(7));
        card.setBackground(criarFundoArredondado("#171C24", 12));
        card.setClickable(true);
        card.setFocusable(true);

        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(62)
        );
        cardParams.topMargin = dp(8);
        card.setLayoutParams(cardParams);

        LinearLayout info = new LinearLayout(this);
        info.setOrientation(LinearLayout.VERTICAL);
        info.setGravity(Gravity.CENTER_VERTICAL);

        LinearLayout.LayoutParams infoParams = new LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.MATCH_PARENT,
                1f
        );
        info.setLayoutParams(infoParams);

        TextView nome = new TextView(this);
        nome.setText(
                servidor.name.isEmpty()
                        ? "Detectando nome..."
                        : servidor.name
        );
        nome.setTextColor(Color.WHITE);
        nome.setTextSize(14);
        nome.setSingleLine(true);
        nome.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        TextView endereco = new TextView(this);
        endereco.setText(servidor.address);
        endereco.setTextColor(Color.parseColor("#818B9A"));
        endereco.setTextSize(10);
        endereco.setSingleLine(true);

        TextView detalhes = new TextView(this);
        detalhes.setText("Consultando...");
        detalhes.setTextColor(Color.parseColor("#626C7A"));
        detalhes.setTextSize(9);
        detalhes.setSingleLine(true);

        info.addView(nome);
        info.addView(endereco);
        info.addView(detalhes);

        TextView status = new TextView(this);
        status.setText("...");
        status.setTextColor(Color.parseColor("#8DB5FF"));
        status.setTextSize(10);
        status.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        status.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                dp(92),
                dp(38)
        );
        status.setLayoutParams(statusParams);

        Button favorito = new Button(this);
        favorito.setText(isFavorito(servidor.address) ? "★" : "☆");
        favorito.setTextSize(18);
        favorito.setTextColor(
                isFavorito(servidor.address)
                        ? Color.parseColor("#FFD166")
                        : Color.parseColor("#8B94A2")
        );
        favorito.setBackgroundTintList(
                ColorStateList.valueOf(Color.parseColor("#171C24"))
        );
        favorito.setPadding(0, 0, 0, 0);
        favorito.setMinWidth(0);
        favorito.setMinHeight(0);

        LinearLayout.LayoutParams favParams = new LinearLayout.LayoutParams(
                dp(42),
                dp(42)
        );
        favorito.setLayoutParams(favParams);

        Button menu = new Button(this);
        menu.setText("...");
        menu.setTextSize(17);
        menu.setTextColor(Color.parseColor("#A9B1BD"));
        menu.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        menu.setBackgroundTintList(
                ColorStateList.valueOf(Color.parseColor("#171C24"))
        );
        menu.setPadding(0, 0, 0, dp(8));
        menu.setMinWidth(0);
        menu.setMinHeight(0);

        LinearLayout.LayoutParams menuParams = new LinearLayout.LayoutParams(
                dp(42),
                dp(42)
        );
        menu.setLayoutParams(menuParams);

        card.addView(info);
        card.addView(status);
        card.addView(favorito);
        card.addView(menu);

        card.setOnClickListener(v -> {
            selectedServerAddress = servidor.address;
            selectedServerName = nome.getText().toString();

            prefs.edit()
                    .putString("server_address", selectedServerAddress)
                    .apply();

            textSubtitle.setText("Selecionado: " + selectedServerName);
            atualizarStatusLauncher();

            Toast.makeText(
                    this,
                    selectedServerName + " selecionado",
                    Toast.LENGTH_SHORT
            ).show();
        });

        favorito.setOnClickListener(v -> {
            boolean agoraFavorito = alternarFavorito(servidor.address);

            favorito.setText(agoraFavorito ? "★" : "☆");
            favorito.setTextColor(
                    agoraFavorito
                            ? Color.parseColor("#FFD166")
                            : Color.parseColor("#8B94A2")
            );

            Toast.makeText(
                    this,
                    agoraFavorito
                            ? "Adicionado aos favoritos"
                            : "Removido dos favoritos",
                    Toast.LENGTH_SHORT
            ).show();

            atualizarFavoritosPreview();

            if (currentSection == 1 && !agoraFavorito) {
                mostrarFavoritos();
            }
        });

        menu.setOnClickListener(v ->
                abrirMenuServidor(
                        menu,
                        servidor,
                        nome.getText().toString()
                )
        );

        serverListContainer.addView(card);

        consultarServidor(
                servidor.address,
                nome,
                status,
                detalhes,
                servidor.name
        );
    }

    private void abrirMenuServidor(
            Button anchor,
            ServerItem servidor,
            String nomeAtual
    ) {
        PopupMenu popup = new PopupMenu(this, anchor);

        popup.getMenu().add("Editar IP / porta");
        popup.getMenu().add("Excluir servidor");

        popup.setOnMenuItemClickListener(item -> {
            String titulo = item.getTitle().toString();

            if ("Editar IP / porta".equals(titulo)) {
                abrirDialogEditarServidor(servidor, nomeAtual);
                return true;
            }

            if ("Excluir servidor".equals(titulo)) {
                confirmarExcluirServidor(servidor, nomeAtual);
                return true;
            }

            return false;
        });

        popup.show();
    }

    private void abrirDialogEditarServidor(
            ServerItem servidor,
            String nomeAtual
    ) {
        LinearLayout conteudo = new LinearLayout(this);
        conteudo.setOrientation(LinearLayout.VERTICAL);
        conteudo.setPadding(dp(24), dp(8), dp(24), 0);

        EditText endereco = new EditText(this);
        endereco.setHint("IP:Porta");
        endereco.setText(servidor.address);
        endereco.setSingleLine(true);
        endereco.setInputType(
                InputType.TYPE_CLASS_TEXT
                        | InputType.TYPE_TEXT_VARIATION_URI
        );

        conteudo.addView(endereco);

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Editar servidor")
                .setMessage(nomeAtual)
                .setView(conteudo)
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("SALVAR", null)
                .create();

        dialog.setOnShowListener(d -> dialog
                .getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String novoEndereco = validarEnderecoServidor(
                            endereco,
                            endereco.getText().toString().trim()
                    );

                    if (novoEndereco == null) {
                        return;
                    }

                    if (servidorJaExisteExceto(
                            novoEndereco,
                            servidor.address
                    )) {
                        Toast.makeText(
                                this,
                                "Esse servidor já está na lista",
                                Toast.LENGTH_SHORT
                        ).show();
                        return;
                    }

                    editarServidor(
                            servidor,
                            novoEndereco
                    );

                    dialog.dismiss();

                    Toast.makeText(
                            this,
                            "Servidor atualizado",
                            Toast.LENGTH_SHORT
                    ).show();

                    if (currentSection == 1) {
                        mostrarFavoritos();
                    } else {
                        mostrarListaServidores();
                    }
                }));

        dialog.show();
    }

    private void editarServidor(
            ServerItem servidor,
            String novoEndereco
    ) {
        String enderecoAntigo = servidor.address;

        if (servidor.custom) {
            List<ServerItem> lista = carregarServidoresPersonalizados();

            JSONArray array = new JSONArray();

            try {
                for (ServerItem item : lista) {
                    JSONObject obj = new JSONObject();

                    if (item.address.equalsIgnoreCase(enderecoAntigo)) {
                        obj.put("name", item.name);
                        obj.put("address", novoEndereco);
                    } else {
                        obj.put("name", item.name);
                        obj.put("address", item.address);
                    }

                    array.put(obj);
                }

                prefs.edit()
                        .putString(PREF_CUSTOM_SERVERS, array.toString())
                        .apply();

            } catch (Exception ignored) {
            }
        } else {
            prefs.edit()
                    .putString(PREF_TEST_SERVER_ADDRESS, novoEndereco)
                    .putBoolean(PREF_TEST_SERVER_HIDDEN, false)
                    .apply();
        }

        Set<String> favoritos = carregarFavoritos();

        if (favoritos.remove(enderecoAntigo)) {
            favoritos.add(novoEndereco);

            prefs.edit()
                    .putStringSet(PREF_FAVORITES, favoritos)
                    .apply();
        }

        if (selectedServerAddress.equalsIgnoreCase(enderecoAntigo)) {
            selectedServerAddress = novoEndereco;

            prefs.edit()
                    .putString("server_address", novoEndereco)
                    .apply();
        }
    }

    private void confirmarExcluirServidor(
            ServerItem servidor,
            String nomeAtual
    ) {
        new AlertDialog.Builder(this)
                .setTitle("Excluir servidor")
                .setMessage(
                        "Excluir \"" + nomeAtual + "\" da sua lista?"
                )
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("EXCLUIR", (dialog, which) -> {
                    excluirServidor(servidor);

                    Toast.makeText(
                            this,
                            "Servidor excluído",
                            Toast.LENGTH_SHORT
                    ).show();

                    if (currentSection == 1) {
                        mostrarFavoritos();
                    } else {
                        mostrarListaServidores();
                    }
                })
                .show();
    }

    private void excluirServidor(ServerItem servidor) {
        String enderecoExcluido = servidor.address;

        if (servidor.custom) {
            List<ServerItem> lista = carregarServidoresPersonalizados();
            JSONArray array = new JSONArray();

            try {
                for (ServerItem item : lista) {
                    if (item.address.equalsIgnoreCase(enderecoExcluido)) {
                        continue;
                    }

                    JSONObject obj = new JSONObject();
                    obj.put("name", item.name);
                    obj.put("address", item.address);
                    array.put(obj);
                }

                prefs.edit()
                        .putString(PREF_CUSTOM_SERVERS, array.toString())
                        .apply();

            } catch (Exception ignored) {
            }

        } else {
            prefs.edit()
                    .putBoolean(PREF_TEST_SERVER_HIDDEN, true)
                    .apply();
        }

        Set<String> favoritos = carregarFavoritos();
        favoritos.remove(enderecoExcluido);

        prefs.edit()
                .putStringSet(PREF_FAVORITES, favoritos)
                .apply();

        if (selectedServerAddress.equalsIgnoreCase(enderecoExcluido)) {
            selecionarPrimeiroServidorDisponivel();
        }
    }

    private void selecionarPrimeiroServidorDisponivel() {
        List<ServerItem> restantes = carregarTodosServidores();

        if (restantes.isEmpty()) {
            selectedServerAddress = "";
            selectedServerName = "";

            prefs.edit()
                    .remove("server_address")
                    .apply();

            textSubtitle.setText(
                    "Adicione ou selecione um servidor"
            );
            return;
        }

        ServerItem primeiro = restantes.get(0);

        selectedServerAddress = primeiro.address;
        selectedServerName = primeiro.name.isEmpty()
                ? primeiro.address
                : primeiro.name;

        prefs.edit()
                .putString("server_address", selectedServerAddress)
                .apply();

        atualizarStatusLauncher();
    }

    private boolean servidorJaExisteExceto(
            String endereco,
            String ignorarEndereco
    ) {
        for (ServerItem servidor : carregarTodosServidores()) {
            if (servidor.address.equalsIgnoreCase(ignorarEndereco)) {
                continue;
            }

            if (servidor.address.equalsIgnoreCase(endereco)) {
                return true;
            }
        }

        return false;
    }


    private String validarEnderecoServidor(
            EditText campo,
            String valor
    ) {
        String endereco = valor == null ? "" : valor.trim();

        if (endereco.isEmpty()) {
            campo.setError("Digite IP:Porta");
            campo.requestFocus();
            return null;
        }

        int separador = endereco.lastIndexOf(':');

        if (separador <= 0 || separador >= endereco.length() - 1) {
            campo.setError("Use o formato IP:Porta");
            campo.requestFocus();
            return null;
        }

        String host = endereco.substring(0, separador).trim();
        String portaTexto = endereco.substring(separador + 1).trim();

        if (host.isEmpty()) {
            campo.setError("IP ou domínio inválido");
            campo.requestFocus();
            return null;
        }

        int porta;

        try {
            porta = Integer.parseInt(portaTexto);
        } catch (Exception e) {
            campo.setError("Porta inválida");
            campo.requestFocus();
            return null;
        }

        if (porta < 1 || porta > 65535) {
            campo.setError("Use uma porta entre 1 e 65535");
            campo.requestFocus();
            return null;
        }

        return host + ":" + porta;
    }

    private void jogarServidorSelecionado() {
        String nick = editNick.getText().toString().trim();

        if (selectedServerAddress == null
                || selectedServerAddress.trim().isEmpty()) {
            Toast.makeText(
                    this,
                    "Selecione ou adicione um servidor primeiro.",
                    Toast.LENGTH_LONG
            ).show();
            return;
        }

        if (nick.isEmpty()) {
            editNick.setError("Digite seu Nick");
            editNick.requestFocus();
            return;
        }

        prefs.edit()
                .putString("nickname", nick)
                .putString("server_address", selectedServerAddress)
                .apply();

        iniciarServidor(selectedServerAddress, selectedServerName);
    }

    private void iniciarServidor(String enderecoServidor, String nomeServidor) {
        String nick = prefs.getString("nickname", "").trim();

        if (nick.isEmpty()) {
            Toast.makeText(
                    this,
                    "Defina seu nickname em CONFIGURAÇÕES primeiro.",
                    Toast.LENGTH_LONG
            ).show();

            mostrarConfiguracoes();
            return;
        }

        prefs.edit()
                .putString("server_address", enderecoServidor)
                .apply();

        Intent intent = new Intent(
                ServersActivity.this,
                SAMP.class
        );

        intent.putExtra("nickname", nick);
        intent.putExtra("server_address", enderecoServidor);

        startActivity(intent);
    }

    private List<ServerItem> carregarTodosServidores() {
        List<ServerItem> lista = new ArrayList<>();

        if (!prefs.getBoolean(PREF_TEST_SERVER_HIDDEN, false)) {
            String enderecoTeste = prefs.getString(
                    PREF_TEST_SERVER_ADDRESS,
                    TEST_SERVER_ADDRESS
            );

            lista.add(new ServerItem(
                    TEST_SERVER_NAME,
                    enderecoTeste,
                    false
            ));
        }

        lista.addAll(carregarServidoresPersonalizados());

        return lista;
    }

    private List<ServerItem> carregarServidoresPersonalizados() {
        List<ServerItem> lista = new ArrayList<>();

        String json = prefs.getString(PREF_CUSTOM_SERVERS, "[]");

        try {
            JSONArray array = new JSONArray(json);

            for (int i = 0; i < array.length(); i++) {
                JSONObject obj = array.getJSONObject(i);

                String nome = obj.optString("name", "").trim();
                String endereco = obj.optString("address", "").trim();

                if (!endereco.isEmpty()) {
                    lista.add(
                            new ServerItem(
                                    nome,
                                    endereco,
                                    true
                            )
                    );
                }
            }
        } catch (Exception ignored) {
        }

        return lista;
    }

    private void salvarServidorPersonalizado(ServerItem novo) {
        List<ServerItem> lista = carregarServidoresPersonalizados();
        lista.add(novo);

        JSONArray array = new JSONArray();

        try {
            for (ServerItem servidor : lista) {
                JSONObject obj = new JSONObject();
                obj.put("name", servidor.name);
                obj.put("address", servidor.address);
                array.put(obj);
            }
        } catch (Exception ignored) {
        }

        prefs.edit()
                .putString(PREF_CUSTOM_SERVERS, array.toString())
                .apply();
    }

    private boolean servidorJaExiste(String endereco) {
        for (ServerItem servidor : carregarTodosServidores()) {
            if (servidor.address.equalsIgnoreCase(endereco)) {
                return true;
            }
        }

        return false;
    }

    private Set<String> carregarFavoritos() {
        Set<String> salvos = prefs.getStringSet(
                PREF_FAVORITES,
                new HashSet<>()
        );

        return new HashSet<>(salvos);
    }

    private boolean isFavorito(String endereco) {
        return carregarFavoritos().contains(endereco);
    }

    private boolean alternarFavorito(String endereco) {
        Set<String> favoritos = carregarFavoritos();

        boolean agoraFavorito;

        if (favoritos.contains(endereco)) {
            favoritos.remove(endereco);
            agoraFavorito = false;
        } else {
            favoritos.add(endereco);
            agoraFavorito = true;
        }

        prefs.edit()
                .putStringSet(PREF_FAVORITES, favoritos)
                .apply();

        return agoraFavorito;
    }

    private void consultarServidor(
            String enderecoServidor,
            TextView nomeView,
            TextView statusView,
            TextView detalhesView,
            String nomeFallback
    ) {
        new Thread(() -> {
            DatagramSocket socket = null;

            try {
                int separador = enderecoServidor.lastIndexOf(':');

                if (separador <= 0 || separador >= enderecoServidor.length() - 1) {
                    throw new Exception("Endereço inválido");
                }

                String host = enderecoServidor.substring(0, separador).trim();
                int porta = Integer.parseInt(
                        enderecoServidor.substring(separador + 1).trim()
                );

                InetAddress inetAddress = InetAddress.getByName(host);
                byte[] ip = inetAddress.getAddress();

                if (ip.length != 4) {
                    throw new Exception("Somente IPv4 nesta versão");
                }

                byte[] query = new byte[11];

                query[0] = 'S';
                query[1] = 'A';
                query[2] = 'M';
                query[3] = 'P';

                query[4] = ip[0];
                query[5] = ip[1];
                query[6] = ip[2];
                query[7] = ip[3];

                query[8] = (byte) (porta & 0xFF);
                query[9] = (byte) ((porta >> 8) & 0xFF);
                query[10] = 'i';

                socket = new DatagramSocket();
                socket.setSoTimeout(1800);

                DatagramPacket envio = new DatagramPacket(
                        query,
                        query.length,
                        inetAddress,
                        porta
                );

                long inicio = System.currentTimeMillis();
                socket.send(envio);

                byte[] resposta = new byte[4096];

                DatagramPacket recebimento = new DatagramPacket(
                        resposta,
                        resposta.length
                );

                socket.receive(recebimento);

                long ping = System.currentTimeMillis() - inicio;
                int tamanho = recebimento.getLength();

                if (tamanho < 20) {
                    throw new Exception("Resposta incompleta");
                }

                int jogadores = lerUnsignedShortLE(resposta, 12);
                int maxJogadores = lerUnsignedShortLE(resposta, 14);

                int tamanhoNome = lerIntLE(resposta, 16);

                if (tamanhoNome < 0
                        || tamanhoNome > 512
                        || 20 + tamanhoNome > tamanho) {
                    throw new Exception("Nome inválido");
                }

                String nomeDetectado = new String(
                        resposta,
                        20,
                        tamanhoNome,
                        Charset.forName("windows-1252")
                ).trim();

                if (nomeDetectado.isEmpty()) {
                    nomeDetectado = nomeFallback.isEmpty()
                            ? enderecoServidor
                            : nomeFallback;
                }

                int offset = 20 + tamanhoNome;
                String idioma = "Idioma não informado";

                // Depois do hostname vêm gamemode e language no query 'i'.
                if (offset + 4 <= tamanho) {
                    int tamanhoGameMode = lerIntLE(resposta, offset);
                    offset += 4;

                    if (tamanhoGameMode >= 0
                            && tamanhoGameMode <= 512
                            && offset + tamanhoGameMode <= tamanho) {
                        offset += tamanhoGameMode;

                        if (offset + 4 <= tamanho) {
                            int tamanhoIdioma = lerIntLE(resposta, offset);
                            offset += 4;

                            if (tamanhoIdioma >= 0
                                    && tamanhoIdioma <= 256
                                    && offset + tamanhoIdioma <= tamanho) {
                                String idiomaDetectado = new String(
                                        resposta,
                                        offset,
                                        tamanhoIdioma,
                                        Charset.forName("windows-1252")
                                ).trim();

                                if (!idiomaDetectado.isEmpty()) {
                                    idioma = idiomaDetectado;
                                }
                            }
                        }
                    }
                }

                final String nomeFinal = nomeDetectado;
                final String idiomaFinal = idioma;
                final String detalheTexto =
                        jogadores + "/" + maxJogadores
                                + " players  •  "
                                + idiomaFinal
                                + "  •  "
                                + ping + " ms";

                runOnUiThread(() -> {
                    nomeView.setText(nomeFinal);

                    statusView.setText("ONLINE");
                    statusView.setTextColor(
                            Color.parseColor("#55D98B")
                    );

                    detalhesView.setText(detalheTexto);
                    detalhesView.setTextColor(
                            Color.parseColor("#7F8998")
                    );
                });

            } catch (Exception e) {
                final String fallback = nomeFallback.isEmpty()
                        ? enderecoServidor
                        : nomeFallback;

                runOnUiThread(() -> {
                    nomeView.setText(fallback);

                    statusView.setText("SEM RESPOSTA");
                    statusView.setTextColor(
                            Color.parseColor("#E8A85A")
                    );

                    detalhesView.setText(
                            "Query indisponível • tente conectar normalmente"
                    );
                    detalhesView.setTextColor(
                            Color.parseColor("#626C7A")
                    );
                });
            } finally {
                if (socket != null) {
                    socket.close();
                }
            }
        }).start();
    }

    private void abrirDiscord() {
        if (DISCORD_URL == null || DISCORD_URL.trim().isEmpty()) {
            Toast.makeText(
                    this,
                    "Link do Discord ainda não configurado.",
                    Toast.LENGTH_SHORT
            ).show();
            return;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(DISCORD_URL));
            startActivity(intent);
        } catch (Exception e) {
            Toast.makeText(
                    this,
                    "Não foi possível abrir o Discord.",
                    Toast.LENGTH_SHORT
            ).show();
        }
    }

    private void prepararContadorLauncher() {
        /*
         * O campo visual já está pronto.
         * O número real precisa vir do seu backend por heartbeat/API.
         * Enquanto o endpoint não estiver ligado, não inventamos contagem.
         */
        if (textLauncherUsers != null) {
            textLauncherUsers.setText("●  — usando o launcher");
            textLauncherUsers.setTextColor(Color.parseColor("#55D98B"));
        }
    }

    private void atualizarStatusLauncher() {
        if (textLauncherStatus == null) {
            return;
        }

        int quantidade = carregarTodosServidores().size();
        boolean selecionado = selectedServerAddress != null
                && !selectedServerAddress.trim().isEmpty();

        String status =
                "✓  Lista carregada (" + quantidade + ")\\n" +
                "✓  Cliente SA-MP integrado\\n" +
                "✓  Loading personalizado\\n" +
                (selecionado
                        ? "✓  Servidor selecionado"
                        : "○  Selecione um servidor");

        textLauncherStatus.setText(status);
    }

    private void atualizarDestaques() {
        if (featuredContainer == null) {
            return;
        }

        featuredContainer.removeAllViews();

        List<ServerItem> servidores = carregarTodosServidores();

        if (servidores.isEmpty()) {
            TextView vazio = new TextView(this);
            vazio.setText("Adicione servidores para aparecerem aqui.");
            vazio.setTextColor(Color.parseColor("#6F7886"));
            vazio.setTextSize(12);
            featuredContainer.addView(vazio);
            return;
        }

        Set<String> favoritos = carregarFavoritos();
        List<ServerItem> ordenados = new ArrayList<>();

        // Favoritos aparecem primeiro como destaques.
        for (ServerItem servidor : servidores) {
            if (favoritos.contains(servidor.address)) {
                ordenados.add(servidor);
            }
        }

        for (ServerItem servidor : servidores) {
            if (!ordenados.contains(servidor)) {
                ordenados.add(servidor);
            }
        }

        int limite = Math.min(3, ordenados.size());

        for (int i = 0; i < limite; i++) {
            adicionarCardDestaque(ordenados.get(i));
        }
    }

    private void adicionarCardDestaque(ServerItem servidor) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(14), dp(10), dp(14), dp(10));
        card.setBackground(criarFundoArredondado("#151B24", 12));
        card.setClickable(true);
        card.setFocusable(true);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                dp(270),
                dp(82)
        );
        params.rightMargin = dp(10);
        card.setLayoutParams(params);

        TextView nome = new TextView(this);
        nome.setText(
                servidor.name.isEmpty()
                        ? "Detectando nome..."
                        : servidor.name
        );
        nome.setTextColor(Color.WHITE);
        nome.setTextSize(13);
        nome.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        nome.setSingleLine(true);

        TextView detalhes = new TextView(this);
        detalhes.setText("Consultando players e idioma...");
        detalhes.setTextColor(Color.parseColor("#768191"));
        detalhes.setTextSize(10);
        detalhes.setSingleLine(true);
        detalhes.setPadding(0, dp(5), 0, 0);

        TextView status = new TextView(this);
        status.setText("...");
        status.setTextColor(Color.parseColor("#8DB5FF"));
        status.setTextSize(9);
        status.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        status.setPadding(0, dp(5), 0, 0);

        card.addView(nome);
        card.addView(detalhes);
        card.addView(status);

        card.setOnClickListener(v -> {
            selectedServerAddress = servidor.address;
            selectedServerName = nome.getText().toString();

            prefs.edit()
                    .putString("server_address", selectedServerAddress)
                    .apply();

            textSubtitle.setText("Selecionado: " + selectedServerName);
            atualizarStatusLauncher();

            Toast.makeText(
                    this,
                    selectedServerName + " selecionado",
                    Toast.LENGTH_SHORT
            ).show();
        });

        featuredContainer.addView(card);

        consultarServidor(
                servidor.address,
                nome,
                status,
                detalhes,
                servidor.name
        );
    }

    private void atualizarFavoritosPreview() {
        if (favoritesPreviewContainer == null) {
            return;
        }

        favoritesPreviewContainer.removeAllViews();

        Set<String> favoritos = carregarFavoritos();
        int adicionados = 0;

        for (ServerItem servidor : carregarTodosServidores()) {
            if (!favoritos.contains(servidor.address)) {
                continue;
            }

            adicionarCardFavoritoPreview(servidor);
            adicionados++;

            if (adicionados >= 4) {
                break;
            }
        }

        if (adicionados == 0) {
            TextView vazio = new TextView(this);
            vazio.setText("Nenhum servidor favoritado.");
            vazio.setTextColor(Color.parseColor("#6F7886"));
            vazio.setTextSize(11);
            vazio.setPadding(dp(2), dp(8), 0, 0);
            favoritesPreviewContainer.addView(vazio);
        }
    }

    private void adicionarCardFavoritoPreview(ServerItem servidor) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(10), dp(6), dp(6), dp(6));
        card.setBackground(criarFundoArredondado("#171C24", 10));

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(58)
        );
        params.topMargin = dp(7);
        card.setLayoutParams(params);

        LinearLayout info = new LinearLayout(this);
        info.setOrientation(LinearLayout.VERTICAL);
        info.setGravity(Gravity.CENTER_VERTICAL);
        info.setLayoutParams(new LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.MATCH_PARENT,
                1f
        ));

        TextView nome = new TextView(this);
        nome.setText(
                servidor.name.isEmpty()
                        ? "Detectando..."
                        : servidor.name
        );
        nome.setTextColor(Color.WHITE);
        nome.setTextSize(11);
        nome.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        nome.setSingleLine(true);

        TextView detalhes = new TextView(this);
        detalhes.setText("Consultando...");
        detalhes.setTextColor(Color.parseColor("#6F7886"));
        detalhes.setTextSize(8);
        detalhes.setSingleLine(true);

        info.addView(nome);
        info.addView(detalhes);

        TextView status = new TextView(this);
        status.setText("...");
        status.setTextColor(Color.parseColor("#8DB5FF"));
        status.setTextSize(8);
        status.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                dp(78),
                LinearLayout.LayoutParams.MATCH_PARENT
        );
        status.setLayoutParams(statusParams);

        card.addView(info);
        card.addView(status);

        card.setOnClickListener(v -> {
            selectedServerAddress = servidor.address;
            selectedServerName = nome.getText().toString();

            prefs.edit()
                    .putString("server_address", selectedServerAddress)
                    .apply();

            textSubtitle.setText("Selecionado: " + selectedServerName);
            atualizarStatusLauncher();
        });

        favoritesPreviewContainer.addView(card);

        consultarServidor(
                servidor.address,
                nome,
                status,
                detalhes,
                servidor.name
        );
    }

    private GradientDrawable criarFundoArredondado(String cor, int raioDp) {
        GradientDrawable fundo = new GradientDrawable();
        fundo.setColor(Color.parseColor(cor));
        fundo.setCornerRadius(dp(raioDp));
        fundo.setStroke(dp(1), Color.parseColor("#222A36"));
        return fundo;
    }

    private int lerUnsignedShortLE(byte[] dados, int offset) {
        return (dados[offset] & 0xFF)
                | ((dados[offset + 1] & 0xFF) << 8);
    }

    private int lerIntLE(byte[] dados, int offset) {
        return (dados[offset] & 0xFF)
                | ((dados[offset + 1] & 0xFF) << 8)
                | ((dados[offset + 2] & 0xFF) << 16)
                | ((dados[offset + 3] & 0xFF) << 24);
    }

    private int dp(int value) {
        return Math.round(
                value * getResources().getDisplayMetrics().density
        );
    }

    private static class ServerItem {
        final String name;
        final String address;
        final boolean custom;

        ServerItem(
                String name,
                String address,
                boolean custom
        ) {
            this.name = name;
            this.address = address;
            this.custom = custom;
        }
    }
}
