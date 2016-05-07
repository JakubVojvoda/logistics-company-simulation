/*
 * Modelovanie a simulacia: system hromadnej obsluhy logistickej firmy
 * Vojvoda Jakub, Bendik Lukas
 * 2013
 *
 * Modelovana strategia c. 2 
 * 
 * Popis:
 * Prijimanie balikov 24/7, pricom vozidla su priradene jednotlivym
 * pobockam podla pociatocneho priradenia.
 * Vozidlo v pobocke naklada baliky, pokym nevycerpa svoju kapacitu
 * alebo nevyprsi casovac, kt. urcuje maximalnu dobu zdrzania.
 * V centrale su baliky vylozene, avsak zacat vykladanie 
 * moze az v momente uvolnenia minimalnej potrebnej kapacity
 * v centralnom sklade (inak vozidla cakaju na uvolnenie).
 * Vozidlo potom prechadza k nakladaniu (strategia 2),
 * kde vybere baliky, ktore patria do pobocky, ku ktorej
 * je auto priradene. Ak sa vsak v centralnom sklade
 * nenachadza ani jeden balik s touto adresou vozidlo
 * sa vracia spat na svoju pobocku.
 * 
 */

#include "libstr.h" 

int T_TRUCK_TIMEOUT_BA;
int T_TRUCK_TIMEOUT_ZA;
int T_TRUCK_TIMEOUT_KE;
int T_TRUCK_TIMEOUT_LV;
int T_TRUCK_TIMEOUT_PP;

// Zariadenia, sklad a fronta urcene pre centralu
Facility central_log_operator("Centrala: spracovatelna balikov");
Facility central_loading_ramp("Centrala: nakladacia rampa");
Facility central_unloading_ramp("Centrala: vykladacia rampa");
Store central("Centrala: sklad", T_CENTRAL_CAPACITY);
Queue central_queue;

// Zariadenia, sklad a fronta urcene pre pobocku
Facility wh_log_operator[T_WAREH_SIZE];
Facility loading_ramp[T_WAREH_SIZE];
Facility unloading_ramp[T_WAREH_SIZE];
Store warehouse[T_WAREH_SIZE];
Queue warehouse_queue[T_WAREH_SIZE];

double petrol_cost;

// statistika zaplnenie aut
Histogram trucksCapacity("Celkova zaplnenie kapacity vozidiel", 0, 3, 11);
Histogram trucksCapacityFromCentral("Zaplnenie kapacity vozidiel: centrala->pobocka", 0, 2, 16);
Histogram trucksCapacityFromBranch("Zaplnenie kapacity vozidiel: pobocka->centrala", 0, 3, 11);

int no_transport = 0;

// statistika doby stravenej v centrale jednotlivymi balikmi
Histogram packBranchHist("Histogram straveneho casu balika v systeme", 0, 120, 20);

/*
 * Funkcia initGlobalScope: 
 *  - urcena k inicializacii zariadeni a skladov
 *    pre jednotlive pobocky - class Warehouse
 */
void initGlobalScope() 
{
  // Firma ma 5 pobociek: BA, ZA, KE, LV, PP
  // pre kazdu je priradene zariadenie a sklad
  wh_log_operator[T_BA].SetName("Pobocka Bratislava: spracovatelna balikov");
  wh_log_operator[T_ZA].SetName("Pobocka Zilina: spracovatelna balikov");
  wh_log_operator[T_KE].SetName("Pobocka Kosice: spracovatelna balikov");
  wh_log_operator[T_LV].SetName("Pobocka Levice: spracovatelna balikov");
  wh_log_operator[T_PP].SetName("Pobocka Poprad: spracovatelna balikov");
  
  loading_ramp[T_BA].SetName("Pobocka Bratislava: nakladacia rampa");
  loading_ramp[T_ZA].SetName("Pobocka Zilina: nakladacia rampa");
  loading_ramp[T_KE].SetName("Pobocka Kosice: nakladacia rampa");
  loading_ramp[T_LV].SetName("Pobocka Levice: nakladacia rampa");
  loading_ramp[T_PP].SetName("Pobocka Poprad:  nakladacia rampa");
  
  warehouse[T_BA].SetName("Pobocka Bratislava: sklad");
  warehouse[T_BA].SetCapacity(T_WAREH_CAPACITY_BA);
  warehouse[T_ZA].SetName("Pobocka Zilina: sklad");
  warehouse[T_ZA].SetCapacity(T_WAREH_CAPACITY_ZA);
  warehouse[T_KE].SetName("Pobocka Kosice: sklad");
  warehouse[T_KE].SetCapacity(T_WAREH_CAPACITY_KE);
  warehouse[T_LV].SetName("Pobocka Levice: sklad");
  warehouse[T_LV].SetCapacity(T_WAREH_CAPACITY_LV);
  warehouse[T_PP].SetName("Pobocka Poprad: sklad");
  warehouse[T_PP].SetCapacity(T_WAREH_CAPACITY_PP);
  
  petrol_cost = 0;
}



Central::Central(int pack_addr, double t) 
        : package_address(pack_addr), arriv_time(t) {
}

void Central::Behavior() {
    
    // Spracovanie balika
    Seize(central_log_operator);
    Wait(T_PROCESS_TIME);
    Release(central_log_operator);
    
    // Ulozenie baliku do centraly, caka na aktivovanie procesom Truck,
    // tzn. na vytiahnutie zo skladu a ulozenie do vozidla
    Enter(central, 1);   
    Into(central_queue);
    Passivate();
    Leave(central, 1);
    
}

int no_war_s = 0;
int no_war_m = 0;
int no_war_e = 0;

Warehouse::Warehouse(int war_addr, int pack_addr) 
          : warehouse_address(war_addr), package_address(pack_addr) {
}

void Warehouse::Behavior() {
    
    //cout << "prichod balika do pobocky " << warehouse_address <<
    //" s ciel. adresou " << package_address << endl;
    
    arriv_time = Time;
    
    no_war_s += 1;
    
    // Spracovanie balika
    Seize(wh_log_operator[warehouse_address]);
    Wait(T_PROCESS_TIME);
    Release(wh_log_operator[warehouse_address]);
    
    no_war_m += 1;
    
    // Ulozenie baliku do skladu, caka na aktivovanie procesom Truck,
    // tzn. na vytiahnutie zo skladu a ulozenie do vozidla
    Enter(warehouse[warehouse_address], 1);
    Into(warehouse_queue[warehouse_address]);
    Passivate();
    Leave(warehouse[warehouse_address], 1);
    
    no_war_e += 1;
}


PackageArrival::PackageArrival(int a, int x) 
              : warehouse_address(a), exp_x(x) {
}

void PackageArrival::Behavior() {
    
    // Generovanie cielovej adresy balika - rovnomerne rozlozenie
    double rr = Uniform(0, 1);
    int address;
    
    if (rr > 0.7) {
      address = T_BA;
    }
    else if (rr > 0.5) {
      address = T_ZA;
    }
    else if (rr > 0.25) {
      address = T_KE;
    }
    else if (rr > 0.15) {
      address = T_LV;
    }
    else { 
      address = T_PP;
    }
    
    // Prichod balikov do pobocky WAREHOUSE_ADDRESS s exp. rozlozenim hp
    (new Warehouse(warehouse_address, address))->Activate();
    Activate(Time + Exponential(exp_x));     
}  

// !!! 2. parameter musi byt v intervale <0, T_WAREH_SIZE> !!!
Truck::Truck(int n, int addr) : id(n), branch_address(addr) {
}

void Truck::Behavior() {
      
    capacity = T_TRUCK_CAPACITY;
    
    start:
    
    // Ak je vozidlo prazdne prejdi rovno k nakladaniu
    if (capacity < T_TRUCK_CAPACITY) {
      
      // Obsadenie vykladacej rampy
      Seize(unloading_ramp[branch_address]);
      
      unload_package:
      
      // Vylozenie balikov po prichode do pobocky
      if (capacity < T_TRUCK_CAPACITY) {
        
        Wait(T_LOADING_TIME);
        //packageTime(Time - package_time.front());
        
        packBranchHist(Time - package_time.front());
        
        package_time.pop();
        
        capacity += 1;
        
        goto unload_package;
      }
      
      // Uvolnenie vykladacej rampy
      Release(unloading_ramp[branch_address]);
    }
    
    // Kapacita vozidla nastavena, tzn. vypraznenie vozidla
    capacity = T_TRUCK_CAPACITY;
    
    // Obsadenie nakladacej rampy
    Seize(loading_ramp[branch_address]); 
    
    // S prichod vozidla na nakladaciu rampu spusti casovac
    int timeout;
    
    // pobocka 0 = Bratislava
    if (branch_address == T_BA)
      timeout = T_TRUCK_TIMEOUT_BA;
    
    // pobocka 1 = Zilina
    else if (branch_address == T_ZA) 
      timeout = T_TRUCK_TIMEOUT_ZA;
    
    // pobocka 2 = Kosice
    else if (branch_address == T_KE) 
      timeout = T_TRUCK_TIMEOUT_KE;
    
    // pobocka 3 = Levice
    else if (branch_address == T_LV)
      timeout = T_TRUCK_TIMEOUT_LV;
    
    // pobocka 4 = Poprad
    else 
      timeout = T_TRUCK_TIMEOUT_PP;
      
    TimerWaiting *tout = new TimerWaiting(this, timeout);
    
    load_package:
    
    // Ak je vo vozidle volna kapacita a nevyprsal casovac, 
    // naloz balik zo skladu
    if (capacity > 0 && !tout->time_to_leave) { 
      
      if (!warehouse[branch_address].Empty() > 0 && !tout->time_to_leave) {
        
        // Nalozenie balika do auta
        Warehouse *w = (Warehouse *)warehouse_queue[branch_address].GetFirst();
        package.push(w->package_address);
        package_time.push(w->arriv_time);
        w->Activate();
        Wait(T_LOADING_TIME);
        capacity -= 1;        
      }
              
      // Prerusenie procesu do doby vyprsania casovaca 
      // alebo moznosti nalozenie dalsieho balika
      WaitUntil(!warehouse_queue[branch_address].Empty() || tout->time_to_leave);
      
      // Ak nevyprsal casovac pokus sa o nalozenie balika
      if (!tout->time_to_leave)
        goto load_package; 
    }
    
    delete tout;                           // Odstranenie casovaca cakania
    Release(loading_ramp[branch_address]); // Uvolnenie nakladacej rampy
    
    trucksCapacity(T_TRUCK_CAPACITY - capacity);
    trucksCapacityFromBranch(T_TRUCK_CAPACITY - capacity);
    no_transport += 1;
    
    // Cesta z pobocky do centraly
    // zasielka z pobocky 0 = Bratislava
    if (branch_address == T_BA) {
      Wait(T_TIME_BA);
      petrol_cost += T_COST_BA; 
    }
    // zasielka z pobocky 1 = Zilina
    else if (branch_address == T_ZA) {
      Wait(T_TIME_ZA);
      petrol_cost += T_COST_ZA;
    }
    // zasielka z pobocky 2 = Kosice
    else if (branch_address == T_KE) {
      Wait(T_TIME_KE);
      petrol_cost += T_COST_KE;
    }
    // zasielka z pobocky 3 = Levice
    else if (branch_address == T_LV) {
      Wait(T_TIME_LV);
      petrol_cost += T_COST_LV;
    }
    // zasielka z pobocky 4 = Poprad
    else {
      Wait(T_TIME_PP);
      petrol_cost += T_COST_PP;
    }
    
    //cerr << capacity << endl;
    //cerr << warehouse[KONTROLOVANA_POBOCKA].Used() << endl;
    
    // Je zbytocne zaberat vykladaciu rampu ak auto prislo bez balikov,
    // preto prechadza rovno k nakladaniu
    if (capacity != T_TRUCK_CAPACITY) {
      
      // Vozidlo cakana pred centralou na uvolnenie kapacity skladu
      // aby bolo mozne zlozit baliky
      WaitUntil((unsigned long)(T_TRUCK_CAPACITY - capacity) <= central.Free());
   
      Seize(central_unloading_ramp);      // Obsadenie vykladacej rampy
      
      c_unload_package:
      
      // Ak prislo vozidlo do pobocky s aspon 1 balikom 
      if (capacity < T_TRUCK_CAPACITY) {
        
        // Vyloz balik do centralneho skladu
        (new Central(package.front(), package_time.front()))->Activate();
        package.pop();
        package_time.pop();
        Wait(T_LOADING_TIME);
        capacity += 1;
        
        goto c_unload_package;
      }
      
      Release(central_unloading_ramp);    // Uvolnenie vykladacej rampy
    }
    
    Seize(central_loading_ramp);        // Obsadenie nakladacej rampy
    
    // Ak je centralny sklad prazdny vyckaj na prichod aspon 1 balika
    if (central_queue.Empty())
      WaitUntil(!central_queue.Empty());
    
    // Ak je kapacita vozidla volna a centralny sklad nie je prazdny
    // inak auto odchadza spat do pobocky
    if (capacity > 0 && !central_queue.Empty()) {
      
      // Pokym je volna kapacita vozidla nakladaj 
      // baliky s rovnakou smerovacou adresou
      // ako je domovska adresa vozidla
      for (Queue::iterator i = central_queue.begin(); 
                           i != central_queue.end() && capacity > 0 ; ++i) {
        
        Central *c = (Central *)central_queue.Get(i); // Vyber z centraly balik

        // Porovnaj adresu vybrateho balika s adresou pobocky,
        // ku ktorej je vozidlo priradene
        // ak sa adresy zhoduju naloz tento balik
        if (branch_address == c->package_address) { 
          
          package_time.push(c->arriv_time);
          c->Activate();
          Wait(T_LOADING_TIME);
          capacity -=1;
          
          // Do vozidla mozu byt nakladane aj prave vylozene baliky z ineho
          // vozidla, ktore presli spracovanim
          i = central_queue.begin();
        }
        // inak balik balik ponechaj v central. sklade
        else {
          central_queue.PostIns(c, i);
        }
        
      }      
    }
    
    // Uvolnenie nakladacej rampy
    Release(central_loading_ramp);
    
    trucksCapacity(T_TRUCK_CAPACITY - capacity);
    trucksCapacityFromCentral(T_TRUCK_CAPACITY - capacity);
    no_transport += 1;
    
    // Cesta z centraly do pobocky
    // zasielka pre pobocky 0 = Bratislava
    if (branch_address == T_BA) {
      Wait(T_TIME_BA);
      petrol_cost += T_COST_BA; 
    }
    // zasielka pre pobocky 1 = Zilina
    else if (branch_address == T_ZA) {
      Wait(T_TIME_ZA);
      petrol_cost += T_COST_ZA;
    }
    // zasielka pre pobocky 2 = Kosice
    else if (branch_address == T_KE) {
      Wait(T_TIME_KE);
      petrol_cost += T_COST_KE;
    }
    // zasielka pre pobocky 3 = Levice
    else if (branch_address == T_LV) {
      Wait(T_TIME_LV);
      petrol_cost += T_COST_LV;
    }
    // zasielka pre pobocky 4 = Poprad
    else {
      Wait(T_TIME_PP);
      petrol_cost += T_COST_PP;
    }
    
    
    // Vykonavanie procesu ma rovnaky scenar s prichodom na pobocku
    goto start;
}


TimerWaiting::TimerWaiting(Truck *t, double tm) 
    : truck(t), timer(tm), time_to_leave(false) {
    // Aktivacia casovaca
    Activate(Time + timer);
}

void TimerWaiting::Behavior() {
    // Ak vyprsal casovac odchodu vozidla z pobocky,
    // nastav premennu odchodu
    time_to_leave = true; 
}


/////////////////////////////////////////////////////////////////////

#define S1_OPTIM_TIMER_BA 280
#define S1_OPTIM_TIMER_KE 240
#define S1_OPTIM_TIMER_ZA 240
#define S1_OPTIM_TIMER_LV 100
#define S1_OPTIM_TIMER_PP 100

#define S1_OPTIMAL 1

int main(/*int argc, char **argv*/) 
{ 
  initGlobalScope();
  SetOutput("strategia2.out");
  
  Init(0, 60*24*365);
  
  T_TRUCK_TIMEOUT_BA = S1_OPTIM_TIMER_BA;
  T_TRUCK_TIMEOUT_ZA = S1_OPTIM_TIMER_ZA;
  T_TRUCK_TIMEOUT_KE = S1_OPTIM_TIMER_KE;
  T_TRUCK_TIMEOUT_LV = S1_OPTIM_TIMER_LV;
  T_TRUCK_TIMEOUT_PP = S1_OPTIM_TIMER_PP;
  
  if (S1_OPTIMAL) {
    
    (new Truck(0, T_BA))->Activate();
    (new Truck(1, T_BA))->Activate();
    (new Truck(2, T_BA))->Activate();
    
    (new Truck(3, T_ZA))->Activate();
    (new Truck(4, T_ZA))->Activate();
    
    (new Truck(5, T_KE))->Activate();
    (new Truck(6, T_KE))->Activate();
    
    (new Truck(7, T_LV))->Activate();
    
    (new Truck(8, T_PP))->Activate();
  }
  
  
  (new PackageArrival(T_BA, T_PACKAGE_TIME_BA))->Activate();
  (new PackageArrival(T_ZA, T_PACKAGE_TIME_ZA))->Activate();
  (new PackageArrival(T_KE, T_PACKAGE_TIME_KE))->Activate();
  (new PackageArrival(T_LV, T_PACKAGE_TIME_LV))->Activate();
  (new PackageArrival(T_PP, T_PACKAGE_TIME_PP))->Activate();
  Run();
  
  
  // Vypis statistik 
  
  for (int i = 0; i < T_WAREH_SIZE; i++)
    warehouse[i].Output();
  
  central.Output();
  
  packBranchHist.Output();
  
  trucksCapacity.Output();
  trucksCapacityFromBranch.Output();
  
  //cout << "Cena benzinu: " << petrol_cost << endl;
  
  return 0;
}
