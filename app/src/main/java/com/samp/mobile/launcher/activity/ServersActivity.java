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

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class ServersActivity extends AppCompatActivity {

    private static final String TEST_SERVER_NAME = "GM Teste";
    private static final String TEST_SERVER_ADDRESS = "179.198.105.167:7125";

    private EditText editNick;
    private EditText editServer;
    private SharedPreferences prefs;

    private TextView textTitle;
    private TextView textSubtitle;
    private LinearLayout serverListContainer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_servers);

        prefs = getSharedPreferences("beta_tester_config", MODE_PRIVATE);

        editNick = findViewById(R.id.edit_nick);
        editServer = findViewById(R.id.edit_server);

        textTitle = findViewById(R.id.text_title);
        textSubtitle = findViewById(R.id.text_subtitle);
        serverListContainer = findViewById(R.id.server_list_container);

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

        // Abre já mostrando a lista.
        mostrarListaServidores();

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

        // Agora SERVIDORES realmente abre/recarrega a lista.
        servidores.setOnClickListener(v -> mostrarListaServidores());

        favoritos.setOnClickListener(v ->
                Toast.makeText(
                        this,
                        "Favoritos será a próxima etapa",
                        Toast.LENGTH_SHORT
                ).show()
        );

        configuracoes.setOnClickListener(v ->
                Toast.makeText(
                        this,
                        "Configurações será a próxima etapa",
                        Toast.LENGTH_SHORT
                ).show()
        );
    }

    private void mostrarListaServidores() {
        textTitle.setText("Servidores");
        textSubtitle.setText("Escolha um servidor ou conecte por IP");

        // Mantém o título "LISTA DE SERVIDORES" do XML.
        while (serverListContainer.getChildCount() > 1) {
            serverListContainer.removeViewAt(1);
        }

        adicionarServidor(
                TEST_SERVER_NAME,
                TEST_SERVER_ADDRESS
        );
    }

    private void adicionarServidor(String nomeServidor, String enderecoServidor) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(18), dp(12), dp(16), dp(12));
        card.setBackgroundColor(Color.parseColor("#171C24"));
        card.setClickable(true);
        card.setFocusable(true);

        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(82)
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
        nome.setText(nomeServidor);
        nome.setTextColor(Color.WHITE);
        nome.setTextSize(17);
        nome.setTypeface(Typeface.DEFAULT, Typeface.BOLD);

        TextView endereco = new TextView(this);
        endereco.setText(enderecoServidor);
        endereco.setTextColor(Color.parseColor("#818B9A"));
        endereco.setTextSize(13);

        TextView detalhes = new TextView(this);
        detalhes.setText("Consultando servidor...");
        detalhes.setTextColor(Color.parseColor("#626C7A"));
        detalhes.setTextSize(11);

        info.addView(nome);
        info.addView(endereco);
        info.addView(detalhes);

        TextView status = new TextView(this);
        status.setText("...");
        status.setTextColor(Color.parseColor("#8DB5FF"));
        status.setTextSize(12);
        status.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        status.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                dp(150),
                dp(46)
        );
        status.setLayoutParams(statusParams);

        card.addView(info);
        card.addView(status);

        card.setOnClickListener(v -> {
            editServer.setText(enderecoServidor);

            Toast.makeText(
                    ServersActivity.this,
                    nomeServidor + " selecionado",
                    Toast.LENGTH_SHORT
            ).show();
        });

        serverListContainer.addView(card);

        consultarServidor(
                enderecoServidor,
                status,
                detalhes
        );
    }

    private void consultarServidor(
            String enderecoServidor,
            TextView statusView,
            TextView detalhesView
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

                // Query "i" = informações básicas do servidor.
                query[10] = 'i';

                socket = new DatagramSocket();
                socket.setSoTimeout(1500);

                DatagramPacket envio = new DatagramPacket(
                        query,
                        query.length,
                        inetAddress,
                        porta
                );

                long inicio = System.currentTimeMillis();
                socket.send(envio);

                byte[] resposta = new byte[2048];

                DatagramPacket recebimento = new DatagramPacket(
                        resposta,
                        resposta.length
                );

                socket.receive(recebimento);

                long ping = System.currentTimeMillis() - inicio;

                int tamanho = recebimento.getLength();

                if (tamanho < 16) {
                    throw new Exception("Resposta incompleta");
                }

                int offset = 12;

                int jogadores = lerUnsignedShortLE(resposta, offset);
                offset += 2;

                int maxJogadores = lerUnsignedShortLE(resposta, offset);

                final String statusTexto = "ONLINE";
                final String detalheTexto =
                        jogadores + "/" + maxJogadores + " jogadores  •  " + ping + " ms";

                runOnUiThread(() -> {
                    statusView.setText(statusTexto);
                    statusView.setTextColor(Color.parseColor("#55D98B"));
                    detalhesView.setText(detalheTexto);
                    detalhesView.setTextColor(Color.parseColor("#7F8998"));
                });

            } catch (Exception e) {
                runOnUiThread(() -> {
                    statusView.setText("OFFLINE");
                    statusView.setTextColor(Color.parseColor("#E86A6A"));
                    detalhesView.setText("Sem resposta do servidor");
                    detalhesView.setTextColor(Color.parseColor("#626C7A"));
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

    private int dp(int value) {
        return Math.round(
                value * getResources().getDisplayMetrics().density
        );
    }
}
