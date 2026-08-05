#pragma once

/*
 * MODELO DE CREDENCIAIS
 *
 * Copie este arquivo para "secrets.h" e preencha com os valores reais:
 *     cp src/secrets.h.model src/secrets.h
 *
 * O secrets.h esta no .gitignore e NAO deve ser commitado.
 */

#define WIFI_SSID "nome-da-rede-wifi"
#define WIFI_PASS "senha-da-rede"

// Host da API (sem http:// e sem porta)
#define API_HOST  "192.168.1.100"

// Porta onde a API responde (8080 no docker-compose, 5226 no dotnet run local)
#define API_PORT  "8080"