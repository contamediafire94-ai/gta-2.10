package com.samp.mobile.launcher.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
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

    private SharedPreferences prefs;

    private TextView textTitle;
    private TextView textSubtitle;
    private LinearLayout serverListContainer;

    private Button buttonServers;
    private Button buttonFavorites;
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
        setContentView(R.layout.activity_servers);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        textTitle = findViewById(R.id.text_title);
        textSubtitle = findViewById(R.id.text_subtitle);
        serverListContainer = findViewById(R.id.server_list_container);

        buttonServers = findViewById(R.id.button_servers);
        buttonFavorites = findViewById(R.id.button_favorites);
        buttonSettings = findViewById(R.id.button_settings);
        buttonPlay = findViewById(R.id.button_play);
        editNick = findViewById(R.id.edit_nick);

        selectedServerAddress = prefs.getString("server_address", TEST_SERVER_ADDRESS);
        editNick.setText(prefs.getString("nickname", ""));

        buttonPlay.setOnClickListener(v -> jogarServidorSelecionado());

        buttonServers.setOnClickListener(v -> mostrarListaServidores());
        buttonFavorites.setOnClickListener(v -> mostrarFavoritos());
        buttonSettings.setOnClickListener(v -> mostrarConfiguracoes());

        mostrarListaServidores();
    }

    private void mostrarListaServidores() {
        currentSection = 0;

        textTitle.setText("Servidores");
        textSubtitle.setText("Escolha, favorite ou adicione um servidor");

        atualizarDestaqueMenu();
        limparLista();
        definirTituloLista("LISTA DE SERVIDORES");
        adicionarBotaoAdicionarServidor();

        for (ServerItem servidor : carregarTodosServidores()) {
            adicionarCardServidor(servidor);
        }
    }

    private void mostrarFavoritos() {
        currentSection = 1;

        textTitle.setText("Favoritos");
        textSubtitle.setText("Seus servidores favoritos");

        atualizarDestaqueMenu();
        limparLista();
        definirTituloLista("SERVIDORES FAVORITOS");

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
                    "Volte em SERVIDORES e toque na estrela ★."
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
        textSubtitle.setText("Ajustes do Wiu Launcher");

        atualizarDestaqueMenu();
        limparLista();
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
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dp(40)
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

        EditText host = new EditText(this);
        host.setHint("IP ou domínio");
        host.setSingleLine(true);
        host.setInputType(
                InputType.TYPE_CLASS_TEXT
                        | InputType.TYPE_TEXT_VARIATION_URI
        );

        EditText porta = new EditText(this);
        porta.setHint("Porta (ex: 7777)");
        porta.setSingleLine(true);
        porta.setInputType(InputType.TYPE_CLASS_NUMBER);

        conteudo.addView(host);
        conteudo.addView(porta);

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Adicionar servidor")
                .setMessage(
                        "O nome será detectado automaticamente pelo servidor."
                )
                .setView(conteudo)
                .setNegativeButton("CANCELAR", null)
                .setPositiveButton("SALVAR", null)
                .create();

        dialog.setOnShowListener(d -> dialog
                .getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String hostServidor = host.getText().toString().trim();
                    String portaTexto = porta.getText().toString().trim();

                    if (hostServidor.isEmpty()) {
                        host.setError("Digite o IP ou domínio");
                        return;
                    }

                    if (portaTexto.isEmpty()) {
                        porta.setError("Digite a porta");
                        return;
                    }

                    int portaServidor;

                    try {
                        portaServidor = Integer.parseInt(portaTexto);
                    } catch (Exception e) {
                        porta.setError("Porta inválida");
                        return;
                    }

                    if (portaServidor < 1 || portaServidor > 65535) {
                        porta.setError("Use uma porta entre 1 e 65535");
                        return;
                    }

                    String endereco = hostServidor + ":" + portaServidor;

                    if (servidorJaExiste(endereco)) {
                        Toast.makeText(
                                this,
                                "Esse servidor já está na lista",
                                Toast.LENGTH_SHORT
                        ).show();
                        return;
                    }

                    salvarServidorPersonalizado(
                            new ServerItem("", endereco, true)
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
        card.setPadding(dp(14), dp(7), dp(6), dp(7));
        card.setBackgroundColor(Color.parseColor("#171C24"));
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
                dp(108),
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
                dp(46),
                dp(42)
        );
        favorito.setLayoutParams(favParams);

        card.addView(info);
        card.addView(status);
        card.addView(favorito);

        card.setOnClickListener(v -> {
            selectedServerAddress = servidor.address;
            selectedServerName = nome.getText().toString();

            prefs.edit()
                    .putString("server_address", selectedServerAddress)
                    .apply();

            textSubtitle.setText("Selecionado: " + selectedServerName);

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

            if (currentSection == 1 && !agoraFavorito) {
                mostrarFavoritos();
            }
        });

        serverListContainer.addView(card);

        consultarServidor(
                servidor.address,
                nome,
                status,
                detalhes,
                servidor.name
        );
    }

    private void jogarServidorSelecionado() {
        String nick = editNick.getText().toString().trim();

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

        lista.add(new ServerItem(
                TEST_SERVER_NAME,
                TEST_SERVER_ADDRESS,
                false
        ));

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
                String[] partes = enderecoServidor.split(":");

                if (partes.length != 2) {
                    throw new Exception("Endereço inválido");
                }

                String host = partes[0].trim();
                int porta = Integer.parseInt(partes[1].trim());

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

                final String nomeFinal = nomeDetectado;
                final String detalheTexto =
                        jogadores + "/" + maxJogadores
                                + " jogadores  •  "
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

                    statusView.setText("OFFLINE");
                    statusView.setTextColor(
                            Color.parseColor("#E86A6A")
                    );

                    detalhesView.setText(
                            "Sem resposta do servidor"
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
