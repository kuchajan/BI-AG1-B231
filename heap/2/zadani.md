Toto zadání je rozšířením příkladu Binární halda I o operace změna hodnoty a smazání prvku. Tyto operace jsou specifické tím, že vyžadují nějakou formu reference na prvek v haldě. Tato reference musí zůstávat validní při pohybu prvku uvnitř haldy.

Jelikož jde o dosti technický a s algoritmizací příliš nesouvisející problém, byla pro vás implementace takovéto reference připravena pomocí tříd Node a Ref. V poli ukládejte Node a konstruktoru Ref předejte referenci na právě vložený Node. Zda ji využijete nebo si napíšete vlastní je na vás.

Úpravy členů BinaryHeap oproti Binární haldě I jsou:

    Typ Ref, který představuje referenci na prvek. Tento typ musí mít defaultní konstruktor a být přesunutelný. Kopírování není vyžadováno. Dále musí implementovat unární operátory * a ->, tedy chovat se jako const pointer na danou hodnotu v haldě.
    push(t) musí vrátit referenci na vložený prvek.
    erase(ref): Smaže prvek určený referencí ref z haldy a vrátí jeho hodnotu.
    change(ref, f): Zavolá funktor f (jeho typ je šablonový parametr) s non-const referencí na prvek určený ref, čímž umožní změnu jeho hodnoty. Funktor f musí být zavolán právě jednou. Po dokončení tohoto volání opraví strukturu haldy.

Časové limity jsou 5 sekund na malý test a 2.1 sekundy na velký.

Nápověda zadarmo: Může být dobrý nápad implementovat pomocnou metodu ref_to_index, která přepočte objekt typu Ref na index daného prvku v haldě.