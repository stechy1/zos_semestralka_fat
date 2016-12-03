#Semestrální práce ZOS 2016
Tématem semestrální práce bude práce s tabulkou pseudoFAT. Vaším cílem bude splnit několik
vybraných úloh.
Základní funkčnost, kterou musí program splňovat. Formát výpisů je závazný.
1. Nahraje soubor z adresáře do cesty virtuální FAT tabulky
```
./program vaseFAT.dat -a s1 ADR
Možný výsledek:
OK
PATH NOT FOUND
```
2. Smaže soubor s1 z vaseFAT.dat (s1 je plná cesta ve virtuální FAT)
```
./program vaseFAT.dat -f s1
Možný výsledek:
OK
PATH NOT FOUND
```
3. Vypíše čísla clusterů, oddělené dvojtečkou, obsahující data souboru s1 (s1 je plná cesta ve
virtuální FAT)
```
./program vaseFAT.dat -c s1
Možný výsledek:
S1 2,3,4,9,15
PATH NOT FOUND
```
4. Vytvoří nový adresář ADR v cestě ADR2
```
./program vaseFAT.dat -m ADR ADR2
Možný výsledek:
OK
PATH NOT FOUND
```
5. Smaže prázdný adresář ADR (ADR je plná cesta ve virtuální FAT)
```
./program vaseFAT.dat -r ADR
Možný výsledek:
OK
PATH NOT FOUND
PATH NOT EMPTY
```
6. Vypíše obsah souboru s1 na obrazovku (s1 je plná cesta ve virtuální FAT)
```
./program vaseFAT.dat -l s1
Možný výsledek:
PATH NOT FOUND
S1: toto je obsah souboru.
```
7. Vypíše obsah adresáře ve formátu +adresář, +podadresář cluster, ukončeno --, - soubor
první_cluster počet_clusterů. Jeden záznam jeden řádek. Podadresáře odsazeny o /t:
```
./program vaseFAT.dat -p
Možný výsledek:
EMPTY
+ROOT
    -S1 1 3
    -S2 5, 1
    +ADRESAR1
    --
    +ADRESAR2
        +ADRESAR3
            -S3 30 5
            -S4 36 1
        --
    --
--
```
## Informace k zadání a omezením
* Maximální počet položek v adresáři bude omezen velikostí 1 clusteru.
* Maximální délka názvu souboru bude 8+3=11 znaků (jméno.přípona) + \0 (ukončovací znak v C/C++), tedy 12 bytů.
* Každý název bude zabírat právě 12 bytů (do délky 12 bytů doplníte \0 - při kratších názvech).
* Každý adresář bude zabírat přesně 1 cluster (i když nebude plně obsazen).
* K dispozici budou testovací FAT soubory, ale je možné si vytvořit nezávisle vlastní
* Protože se bude pracovat s velkým objemem dat, je potřeba danou úlohu paralelizovat - zpracovávat ve více vláknech a tím pádem řešit i patřičnou synchronizaci.
* V dokumentaci bude vhodné provést diskuzi nad výsledkem paralelního zpracování různým
počtem vláken a nalézt optimální stav
Dle Vašeho prvního písmene loginu splňte následující zadání:

První písmeno loginu a-i včetně
* Zkontrolujte, zda je souborový systém nepoškozen.
* V případě poškození zobrazte nalezené chyby a opravte je.

První písmeno loginu j-r včetně
* Proveďte plnou defragmentaci souborového systému.
* Vypište stav před a po defragmentaci (postačí například X/0/- Adresář/Soubor/Prázdné)

První písmeno loginu s-z včetně
* Kontrola a oprava badblocků (přepsat do jiného umístění, změna FAT řetězce).
* Badblock bude rozlišitelný specifickými znaky FFFFFF na začátku a na konci datového bloku (odpovídá několikanásobnému pokusu o čtení daného bloku). Tyto znaky NAHRADÍ původní obsah – tedy dojde k částečné ztrátě původních dat, ale délka v clusteru a adresáři se
nezmění.

## Odevzdání práce
Práci včetně dokumentace pošlete svému cvičícímu e-mailem. V případě velkého objemu dat můžete
využít různé služby (leteteckaposta.cz, uschovna.cz).

Osobní předvedení práce cvičícímu. Referenčním strojem je školní PC v UC326. Práci můžete ukázat
ale i na svém notebooku. Konkrétní datum a čas předvedení práce si domluvte e-mailem se cvičícím,
sdělí vám časová okna, kdy můžete práci ukázat.

Do kdy musím semestrální práci odevzdat?

Zápočet musíte získat do mezního data pro získání zápočtu (10. února 2017).

A samozřejmě musíte mít zápočet dříve, než půjdete na zkoušku (alespoň 1 den předem).
## Hodnocení
Při kontrole semestrální práce bude hodnocena:
* Kvalita a čitelnost kódu včetně komentářů
* Funkčnost a kvalita řešení (výsledné časy, přístup k paralelnímu zpracování)
* Dokumentace
0
