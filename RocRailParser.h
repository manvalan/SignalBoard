#ifndef ROCRAIL_PARSER_H
#define ROCRAIL_PARSER_H

#include <Arduino.h>
#include "GestoreScambi.h"
#include "MappaturaScambi.h"

class RocrailParser {
  private:
    GestoreScambi& _hardware;

  public:
    RocrailParser(GestoreScambi& hardware) : _hardware(hardware) {}

    String interpretaXML(String msg) {
      if (msg.indexOf("<sw ") == -1) return "";
      
      int addrStart = msg.indexOf("addr1=\"");
      if (addrStart != -1) {
        addrStart += 7; 
        int addrEnd = msg.indexOf("\"", addrStart);
        int idScambioRocrail = msg.substring(addrStart, addrEnd).toInt();

        String idTestuale = "";
        int idStart = msg.indexOf("id=\"");
        if (idStart != -1) {
          idStart += 4;
          int idEnd = msg.indexOf("\"", idStart);
          idTestuale = msg.substring(idStart, idEnd);
        }

        int cmdStart = msg.indexOf("cmd=\"");
        if (cmdStart != -1) {
          cmdStart += 5; 
          int cmdEnd = msg.indexOf("\"", cmdStart);
          String comando = msg.substring(cmdStart, cmdEnd);

          int canaleDaAttivare = -1;
          bool trovato = false;

          // DEBUG: Stampa cosa è arrivato da Rocrail
          Serial.printf("[Parser] Ricevuto XML per Scambio ID: %s, Indirizzo Rocrail: %d, Comando: %s\n", idTestuale.c_str(), idScambioRocrail, comando.c_str());

          for (int i = 0; i < NUMERO_MOTORI; i++) {
            if (TABELLA_CABLAGGIO[i].indirizzoRocrail == idScambioRocrail) {
              trovato = true;
              if (comando == "straight") {
                canaleDaAttivare = TABELLA_CABLAGGIO[i].canaleDritto;
              } else if (comando == "turnout") {
                canaleDaAttivare = TABELLA_CABLAGGIO[i].canaleDeviata;
              }
              break;
            }
          }

          if (trovato && canaleDaAttivare != -1) {
            // DEBUG: Stampa quale canale della MappaturaScambi.h è stato associato
            Serial.printf("[Parser] Mappatura trovata! Associo a Canale Mux: C%d\n", canaleDaAttivare);
            
            _hardware.attivaCanaleFisico(canaleDaAttivare);

            String feedbackXML = "<sw id=\"" + idTestuale + "\" addr1=\"" + String(idScambioRocrail) + "\" cmd=\"" + comando + "\"/>";
            return feedbackXML;
          } else {
            // DEBUG: Avvisa se lo scambio non è censito nella tabella
            Serial.printf("[Parser] ATTENZIONE: Indirizzo Rocrail %d non trovato in TABELLA_CABLAGGIO!\n", idScambioRocrail);
          }
        }
      }
      return ""; 
    }
};

#endif