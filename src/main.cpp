// latency_rx – přijímač testovacích paketů pro měření latence síťového stacku.

// Princip: program čeká na testovací UDP paket. Jakmile ho dostane,
// OKAMŽITĚ přepne výstup GPIO. Pico pak změří čas mezi příchodem rámce
// na kabel (senzor) a touto hranou na GPIO.

// Zlaté pravidlo: mezi návratem z recv() a přepnutím GPIO nesmí být nic
// dalšího – žádný výpis, žádná kontrola, žádná alokace. Všechno, co tam
// dáš, se započítá do měřené latence.
int main(int argc, char *argv[])
{

}