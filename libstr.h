/*
 * Modelovanie a simulacia: system hromadnej obsluhy logistickej firmy
 * Vojvoda Jakub, Bendik Lukas
 * 2013
 *
 */

#include <simlib.h>
#include <iostream>
#include <queue>
#include <math.h>

#include <sstream>
#include <string>

// Makro pre vypis debug-ovacich informacii: make debug
#ifdef DEBUG
#define _DEBUG if(1)
#else
#define _DEBUG if(0)
#endif


// 2x 00
#define T_CENTRAL_CAPACITY 500000   // Kapacita centralneho skladu

// Kapacita skladu jednotlivych pobociek
#define T_WAREH_CAPACITY_BA 500000
#define T_WAREH_CAPACITY_ZA 500000
#define T_WAREH_CAPACITY_KE 500000
#define T_WAREH_CAPACITY_LV 500000
#define T_WAREH_CAPACITY_PP 500000

#define T_WAREH_SIZE     5      // Pocet pobociek
#define T_TRUCK_CAPACITY 30    // Kapacita vozidla 30

// 5 pobociek: BA, ZA, KE, LV, SVIT
#define T_BA   0
#define T_ZA   1
#define T_KE   2
#define T_LV   3
#define T_PP   4

// benzin cena na 1 km <= 10.7e / 100 km 
#define T_PETROL_COST  0.107
//#define T_PETROL_COST  0.0634

// cas medzi pobockou a centralou (v minutach)
#define T_TIME_BA   126 + Exponential(12.6)  
#define T_TIME_ZA   104 + Exponential(10.4)  
#define T_TIME_KE   181 + Exponential(18.1)   
#define T_TIME_LV   80 + Exponential(8)
#define T_TIME_PP   94 + Exponential(9.4)

// vzdialenost medzi pobockou a centralou (v km)
#define T_DIST_BA   224 + Exponential(22.4)
#define T_DIST_ZA   90 + Exponential(9)
#define T_DIST_KE   235 + Exponential(23.5)
#define T_DIST_LV   111 + Exponential(11.1)
#define T_DIST_PP   95 + Exponential(9.5)

// cena benzinu za 1 cestu
#define T_COST_BA   0.107 * (224 + Exponential(22.4))
#define T_COST_ZA   0.107 * (90 + Exponential(9))
#define T_COST_KE   0.107 * (235 + Exponential(23.5))
#define T_COST_LV   0.107 * (111 + Exponential(11.1))
#define T_COST_PP   0.107 * (95 + Exponential(9.5))

// prichod balikov na jednotlive pobocky
#define T_PACKAGE_TIME_BA   7.5
#define T_PACKAGE_TIME_ZA   37.5
#define T_PACKAGE_TIME_KE   12.5
#define T_PACKAGE_TIME_LV   75
#define T_PACKAGE_TIME_PP   75

// casy nalozenia/vylozenia a spracovania balikov
#define T_LOADING_TIME Uniform(1, 2)
#define T_PROCESS_TIME Uniform(1, 3)

using namespace std;


void initGlobalScope(); 


class Central : public Process {
  
public:
  int package_address;  // Cielova adresa balika
  double arriv_time;
  
  Central(int pack_addr, double t);
  
  void Behavior();
  
};

/* 
 * Trieda Warehouse:
 * - predstavujuca sklad pobocky 
 * - do pobocky prichadzaju jednotlive baliky 
 *   a po spracovani su uloze do skladu, kde
 *   cakaju na dalsie spracovanie
 */
class Warehouse : public Process {
  
public:
  int warehouse_address;        // Adresa pobocky
  int package_address;          // Cielova adresa balika
  double arriv_time;
  
  Warehouse(int war_addr, int pack_addr);
  
  void Behavior();
  
};


/* 
 * Trieda PackageArrival:
 * - predstavujuca prichod balikov do systemu 
 */
class PackageArrival : public Event {
  
public:
  int warehouse_address;  // Adresa pobocky (skladu)
  int exp_x;              // Stred exp. r. prichodu balikov
                          // na pobocku warehouse_address
  
  PackageArrival(int a, int x);
  
  void Behavior();
  
};  

/*
 * Trieda Truck:
 * - predstavujuca proces auta s urcitou capacitou,
 *   kt. sa stara o rozvoz balikov medzi pobockami
 *   a centralnym skladom
 */
class Truck : public Process {

public:  
  int capacity;         // Volna kapacita vozidla  
  int id;               // ID vozidla (len pre DEBUG)
  queue<int> package;   // Adresy balikov, kt. prevaza
  int branch_address;   // Adresa pobocky - pociatocne priradenie
                        // pobocky (nastavovane pri kazdej jazde)
  queue<double> package_time;
  
  Truck(int n, int addr); 
  
  void Behavior();

};


class TimerPackage : public Event {

public:
  int warehouse_address;
  int package_address;
  
  TimerPackage(int war_addr, int pack_addr);
  
  void Behavior();
};


/*
 * Trieda TimerWaiting:
 * - predstavujuca casovu udalost, kedy urcite vozidlo
 *   je nutene opustit system 
 * - ak casovac je aktivovany v case nakladania balika,
 *   auto pocka na dokoncenie a az potom opusta system
 */
class TimerWaiting : public Event {
  
public:  
  Truck *truck;         // Process vozidla
  double timer;         // Doba casovaca 
  bool time_to_leave;   // Nastaveny po vyprsani casovaca
  
  TimerWaiting(Truck *t, double tm);
 
  void Behavior();
};

