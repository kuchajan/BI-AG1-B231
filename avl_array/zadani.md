Vaším úkolem je implementovat šablonu třídy Array parametrizovanou typem ukládaných prvků T. Tato třída má rozhraní jako pole, ale měla by udržovat prvky ve struktuře AVL stromu, čímž dosáhne rychlého vkládání a mazání na libovolné pozici za cenu drobného zpomalení přístupu. Vyžadované metody jsou:

    size(): Vrátí počet prvků v poli.
    operator[](i): Vrátí prvek na pozici i. Pro pozici mimo rozsah [0, size()) způsobí výjimku std::out_of_range.
    insert(i, t): Vloží prvek t na pozici i. Prvkům na pozici i a vyšší zvýší index o 1. Pro pozici mimo rozsah [0, size()] způsobí výjimku std::out_of_range.
    erase(i): Smaže prvek na pozici i. Pro pozici mimo rozsah [0, size()) způsobí výjimku std::out_of_range.

Kopírování ani přesouvání objektů typu Array není testováno. Pro účely testování je třeba implementovat následující statické metody třídy Array::TesterInterface:

    root(array): Vrátí pointer na kořen stromu. (Na uzel, nikoliv na hodnotu v něm uloženou.)
    parent(n): Vrátí pointer na otce uzlu n. Pro kořen vrátí nullptr. Pokud vaše implementace nemá ukazatele na otce a máte nastaveno config::PARENT_POINTERS na false, můžete metodu smazat nebo vždy vracet nullptr.
    right(n): Vrátí pointer na pravého syna n nebo nullptr, pokud neexistuje.
    left(n): Vrátí pointer na levého syna n nebo nullptr, pokud neexistuje.
    value(n): Vrátí const referenci na hodnotu uloženou v uzlu n.

Pro snazší vývoj jsou v šabloně k dispozici následující konfigurační volby:

    config::CHECK_DEPTH: Pokud je true, zapne kontrolu tvaru AVL stromu, jinak je pouze kontrolováno, že strom je korektní binární. Progtest tuto volbu ignoruje. Defaultně false, zapněte až implementujete vyvažování.
    config::PARENT_POINTERS: Určuje, zda se mají testovat i ukazatele na otce. Progtest tuto volbu respektuje. Defaultně true.

Pozor: Testování struktury stromu je rekurzivní, a tedy jisté testy mohou pro nevyvažované stromy skončit pádem z důvodu přetečení zásobníku.

Na testy s paměťovým debuggerem je 5 sekund, na ostatní po 6 sekundách.

Hint: Doporučujeme nejprve vypracovat miniprogtest AVL strom a při implementaci z něj vycházet.