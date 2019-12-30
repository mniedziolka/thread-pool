# Asynchroniczne C

## Wprowadzenie
Pula watkow to mechanizm pozwalajacy na uzyskanie wspolbieznego wykonywanie wielu zadan w ramach jednego programu.
W sklad puli wchodzi pewna liczba watkow roboczych (ang. worker threads), czekajacych na pojawienie sie pracy do wykonania.

Uzycie puli watkow pozwala uniknac ciaglego powolywania i czekania na zakonczenie sie watku przy wykonywaniu
krotkotrwalych zadan wspolbieznych. Pozwala tez na wykonywanie bardzo duzej liczby zadan niezaleznie od siebie w sytuacji,
gdy liczba dostepnych potokow przetwarzania jest ograniczona.

## Polecenie
* Zaimplementuj pule watkow zgodnie z ponizszym opisem szczegolowym (3 pkt).

* Zaimplementuj obliczenia future zgodnie z ponizszym opisem szczegolowym (3 pkt).

* Napisz program przykladowy macierz, obliczajacy za pomoca puli watkow sumy wierszy z zadanej tablicy (1 pkt).

* Napisz program przykladowy silnia, obliczajacy za pomoca mechanizmu future silnie zadanej liczby (1 pkt).

* Zadbaj, aby kod byl napisany w sposob klarowny i rzetelny zgodnie z ponizszymi wytycznymi. (2 pkt).

## Szczegolowy opis puli watkow
Pule watkow nalezy zaimplementowac jako realizacje interfejsu przedstawionego w pliku "threadpool.h". Zamieszczone tam sa m.in. nastepujace deklaracje:

```C
typedef struct runnable {
  void (*function)(void *, size_t);
  void *arg;
  size_t argsz;
} runnable_t;

typedef struct thread_pool {

} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);
```
Wywolanie thread_pool_init inicjuje argument wskazywany przez pool jako nowa pule, w ktorej bedzie funkcjonowac
pool_size watkow obslugujacych zgloszone do wykonania zadania. Za gospodarke pamiecia wskazywana przez pool odpowiada
uzytkownik biblioteki. Poprawnosc dzialania biblioteki jest gwarantowana tylko, jesli kazda pula stworzona przez
thread_pool_init jest niszczona przez wywolanie thread_pool_destroy z argumentem reprezentujacym te pule.

Wywolanie defer(pool, runnable) zleca puli watkow pool wykonanie zadania opisanego przez argument runnable, argumenty
function sa przekazywane przez wskaznik args, w polu argsz znajduje sie dlugosc dostepnego do pisania i czytania buforu
znajdujacego sie pod tym wskaznikiem. Za zarzadzanie pamiecia wskazywana przez args odpowiada klient biblioteki.

Funkcja function powinna zostac obliczona przez watek z puli pool; wywolanie defer moze zablokowac wywolujacy je watek,
ale jedynie na potrzeby rejestracji zlecenia: powrot z defer jest niezalezny od powrotu z wykonania function przez pule.

Zadania zlecone do wykonania przez defer powinny moc wykonywac sie wspolbieznie i na tyle niezaleznie od siebie, na ile
to mozliwe. Mozna ograniczyc liczbe wspolbieznie wykonywanych zadan do rozmiaru puli. Pula w czasie swojego dzialania nie
powinna powolywac wiecej watkow niz okreslono parametrem pool_size. Utworzone watki sa utrzymywane az do wywolania
thread_pool_destroy.

Zastanow sie nad tym, jak zrealizowac powyzsza biblioteke tak, aby wykonywala sie mozliwie sprawnie na wspolczesnych
komputerach. Postaraj sie zrealizowac taka implementacje.

## Szczegolowy opis mechanizmu obliczen future
Przy pomocy puli watkow i operacji defer nalezy zaimplenentowac asynchroniczne obliczenia future jako realizacje interfejsu przedstawionego w pliku "future.h". Zamieszczone sa tam m.in. nastepujace deklaracje:

```C
typedef struct callable {
  void *(*function)(void *, size_t, size_t *);
  void *arg;
  size_t argsz;
} callable_t;

typedef struct future {
} future_t;

int async(thread_pool_t *pool, future_t *future, callable_t callable);

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *));

void *await(future_t *future);
```
Wywolanie int err = async(pool, future_value, callable) inicjuje pamiec wskazywana przez future_value.
Za zarzadanie ta pamiecia odpowiada uzytkownik biblioteki. Na puli pool zlecane jest wykonanie function z argumentu
callable. Funkcja function zwraca wskaznik do wyniku. Uzytkownik biblioteki powinien zadbac, zeby poprawnie ustawila tez
rozmiar wyniku wykorzystujac do tego celu trzeci argument typu size_t*.

Wolajacy moze teraz:

* Zaczekac na zakonczenie wykonania funkcji function przez wywolanie:
```C
void *result = await(future_value);
```
Za gospodarke pamiecia wskazywana przez wskaznik result odpowiada uzytkownik biblioteki (pamiec ta moze zostac przekazana
do funkcji function za pomoca jej argumentow lub w tej funkcji zaalokowana).

* Zlecic jakiejs puli, niekoniecznie tej, ktora zainicjowala future_value, wywolanie innej funkcji na wyniku:
```C
err = map(pool2, mapped_value, future_value, function2);
```

Programy, w ktorych aktywnie dziala jakas pula watkow, powinny miec automatycznie ustawiona obsluge sygnalow.
Ta obsluga powinna zapewniac, ze program po otrzymaniu sygnalu (SIGINT) zablokuje mozliwosc dodawania nowych zadan do
dzialajacych pul, dokonczy wszystkie obliczenia zlecone dotad dzialajacym pulom, a nastepnie zniszczy dzialajace pule.

Dla ulatwienia implementacji mozna zalozyc, ze zaimplementowana biblioteka bedzie testowana w taki sposob, iz watki
nie beda ginely w testach.

## Opis programu macierz
Program macierz ma ze standardowego wejscia wczytywac dwie liczby k oraz n, kazda w osobnym wierszu. Liczby te oznaczaja
odpowiednio liczbe wierszy oraz kolumn macierzy. Nastepnie program ma wczytac k*n linijek z danymi, z ktorych kazda
zawiera dwie, oddzielone spacja liczby: v, t. Liczba v umieszczona w linijce i (numeracje linijek zaczynamy od 0)
okresla wartosc macierzy z wiersza floor(i/n) (numeracje kolumn i wierszy zaczynamy od 0) oraz kolumny i mod n.
Liczba t to liczba milisekund, jakie sa potrzebne do obliczenia wartosci v. Oto przykladowe poprawne dane wejsciowe:

```
2
3
1 2
1 5
12 4
23 9
3 11
7 2
```

Takie dane wejsciowe tworza macierz od dwoch wierszach i trzech kolumnach:
```
|  1  1 12 |
| 23  3  7 |
```

Program ma za zadanie wczytac tak sformatowane wejscie (mozna zakladac, ze podawane beda tylko poprawne dane),
a nastepnie za pomoca puli watkow zawierajacej 4 watki policzyc sumy wierszy, przy czym pojedyncze zadanie obliczeniowe
powinno podawac w wyniku wartosc pojedynczej komorki macierzy, odczekawszy liczbe milisekund, ktore zostaly wczytane
jako potrzebne do obliczenia tej wartosci (np. zadanie obliczeniowe wyliczenia wartosci 3 z macierzy powyzej powinno
odczekiwac 11 milisekund). Po obliczeniu nalezy wypisac sumy kolejnych wierszy na standardowe wyjscie, po jednej sumie
w wierszu. Dla przykladowej macierzy powyzej umieszczonej w pliku data1.dat wywolanie:

```shell script
$ cat data1.dat | ./macierz
```
powinno spowodowac pojawienie sie na wyjsciu
```
14
33
```

## Opis programu silnia
Program silnia powinien wczytywac ze standardowego wejscia pojedyncza liczbe n, a nastepnie obliczac za pomoca puli 3
watkow liczbe n!. Po obliczeniu tej liczby wynik powinien zostac wypisany na standardowe wyjscie. Program powinien
wyliczac silnie, wykorzystujac funkcje map i przekazujac jej w future_value czesciowe iloczyny. Dla przykladu wywolanie:
```shell script
$ echo 5 | ./silnia
```
powinno spowodowac pojawienie sie na wyjsciu
```
120
```

## Wymagania techniczne
Do synchronizacji mozna korzystac tylko z mechanizmow biblioteki pthreads. Mozna korzystac z plikow naglowkowych:

```C
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
```
Powyzsza lista moze ulec rozszerzeniu, jesli okaze sie to konieczne. Mozna zalozyc,
ze kod bedzie kompilowany i testowany na serwerze students.

Jako rozwiazanie nalezy wyslac na moodla plik ab123456.tar.gz, gdzie ab123456 to login na students.
W archiwum powinien znalezc sie jeden katalog o nazwie ab123456 (login na students) z wszystkimi plikami rozwiazania.
Kod programow przykladowych nalezy umiescic w plikach macierz.c i silnia.c. Nie powinna byc konieczna modyfikacja
plikow CMakeLists.txt. Ciag polecen:

```shell script
tar -xf ab123456.tar.gz
mkdir build && cd build
cmake ../ab123456
make
```

Powinien skompilowac biblioteke do pliku build/libasyncc.a, kompilator nie powinien wypisywac zadnych ostrzezen.
Nastepnie mozna przetestowac swoje rozwiazanie:

```shell script
make test
```

W czasie oceniania zawartosc katalogu test zostanie podmieniona.

