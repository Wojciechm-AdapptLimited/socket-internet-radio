# Internet Radio over IP

### 1. Autorzy

* Wojciech Mączka 148106
* Iga Pietrusiewicz 148162

### 2. Opis

Celem projektu było stworzenie radia internetowego komunikującego się równolegle z
wieloma klientami, używając API bsd-sockets. Wprowadzone dla naszego radia funkcjonalności
obejmują:

* Strumieniowanie utworów do wielu klientów równolegle
* Przyjmowanie poleceń od klientów (pomiń, dodaj z listy, usuń z kolejki, zmień porządek
  w kolejce) i synchronizacja między klientami
* Aplikację klienta wraz z CLI

Do stworzenia zarówno serwera, jak i klienta wykorzystano język C/C++ i API bsd-sockets.

### 3. Komunikacja

* Przed dołączeniem nowego klienta to listy słuchaczy, serwer wysyła podstawowe informacje
  (stan kolejki, lista plików poza kolejką, rozmiar bieżąco odtwarzanego pliku, jego
  nazwa i nagłówek)
* Nowy klient po otrzymaniu podstawowych informacji dołączony jest do listy słuchaczy,
  do których strumieniowane są kolejne fragmenty pliku.
* Przed rozpoczęciem strumieniowania kolejnego pliku, serwer przesyła do wszystkich klientów
  informacje na jego temat
* Klienci mają możliwość wysyłania żądań na serwer, po których wykonaniu rozsyłany jest
  bieżący stan kolejki
* Przy rozłączaniu się, klient przesyła pakiet o tym informujący

### 4. Implementacja

* Serwer:
    * Implementacja klas: AudioStreamer (odpowiedzialny za odczytywanie plików i
      zarządzanie kolejką), ClientManager (odpowiedzialny za zarządzanie listą klientów
      i rozsyłanie im pakietów), Client (będący abstrakcją socket i odpowiedzialny za jej
      zamknięcie)
    * 3 wątki: 1. Przyjmowanie klientów, 2. Odczytywanie plików i broadcasting, 3.
      Przyjmowanie żądań od klientów i ich realizacja
    * Nasłuchiwanie na sygnał SIGINT i SIGTERM, poprawne zamykanie programu
* Klient:
    * Implementacja klas/struktur: MusicPlayer (abstrakcja obsługi biblioteki irrklang),
      NetworkTraffic (kontrola pakietów wchodzących i wychodzących), SessionData (przechowywanie
      wszystkich potrzebnych obiektów i struktur danych w jednej strukturze)
    * 3 wątki: 1. Wysyłanie i przyjmowanie pakietów, 2. Obsługa pakietów przychodzących, 3.
      CLI i wysyłanie żądań
    * Interfejs umożliwia podgląd bieżącego oraz następnego utworu i wysyłanie wszystkich żądań
    * Odtwarzanie muzyki odbywa się z wykorzystaniem biblioteki irrklang (choć ma ona
      czasami występujący problem z odtwarzaniem utworu zaraz po dołączeniu do serwera, słychać wtedy szum)
* Zarówno klient, jak i serwer wykorzystują strukturę Package, która symbolizuje jeden pakiet
  i przechowuje typ, rozmiar danych i dane

### 5. Uruchomienie

* Zbudowanie projektu
    ```shell
    mkdir build
    cd build
    cmake ..
    make
    ```   
* Następnie (w osobnych oknach terminala)
    ```shell
    ./internet_radio_server
    ```
    ```shell
    ./internet_radio_client
    ```
* Uwaga: przy odpalaniu klienta występuje bug (prawdopodobnie związany z biblioteką
  irrklang), przez który zamiast utworu słyszymy szum. Najlepiej spróbować wtedy zrestartować klienta
  lub przewinąć do następnego utworu