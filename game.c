#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#define MIN_NAME_LENGTH 3
#define MAX_NAME_LENGTH 20
#define ENEMY_DIALOGUES 3

#define TEXT_WAIT 30000
#define NEWLINE_WAIT 300000

#define WEAPON_NUM 5
#define ARMOR_NUM 4
#define SKILL_NUM 2
#define ITEM_NUM 3
#define ENEMY_NUM 5

#define PLAYER_DEF 10
#define PLAYER_DODGE 10
#define PLAYER_CRIT_CH 5
#define PLAYER_START_CRIT_DMG 110
#define PLAYER_INCREASE_CRIT_DMG 10
#define PLAYER_START_HP 100
#define PLAYER_INCREASE_HP 50
#define PLAYER_START_EHP 100
#define PLAYER_INCREASE_EHP 50
#define PLAYER_START_DMG 15
#define PLAYER_INCREASE_DMG 1

#define ENEMY_UPGRADE_FACTOR 1.1

#define YES_NO_TITLE_SIZE 640

#define FAKE_NAME "bastone magicissimo"

#define PERCENT_FORMULA(stat,obj) 50.0/100*pow(obj.lv,(float)1/3)*stat*(100-stat)/(obj.lv*999)

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef struct menu{
	char* title;
	int opt_num;
	char** options;
} menu;

typedef struct weapon{
	uchar id;
	char* name;
	char* desc;
	ushort lv; // se il livello e' 0, l'arma non e' posseduta
	ushort price;
	ushort upgr_price;
	
	// aumento delle statistiche (da sommare a quelle del giocatore)
	int atk;
	float dodge_chance;
	float crit_chance;
	float crit_dmg;
} weapon;
typedef struct armor{
	uchar id;
	char* name;
	char* desc;
	ushort lv; // se il livello e' 0, l'armatura non e' posseduta
	ushort price;
	ushort upgr_price;
	
	// aumento delle statistiche (da sommare a quelle del giocatore)
	float def;
	float dodge_chance; // riduce l'agilita' (le armture sono pesanti), quindi sara' un depotenziamento alla probabilita' di schivata
} armor;
typedef struct skill{
	uchar id;
	char* name;
	char* desc;
	ushort lv; // se il livello e' 0, l'abilita' non e' posseduta
	ushort price;
	ushort upgr_price;
	
	uchar strong_vs; // contro quale stato emotivo e' piu' efficace l'abilita'
	bool special; // se l'abilita' ha un effetto speciale non rappresentabile con un numero (es. cambia lo stato emotivo)
	
	// de/potenziamenti alle stastistiche
	short ehp; //aumenta l'autostima del giocatore
	short edmg; //danno emotivo al nemico
	short enm_atk; //de/potenziamento all'attacco del nemico
	float dodge_chance; //de/potenziamento alla statistica del nemico
	float crit_chance;  //de/potenziamento alla statistica del nemico
	float crit_dmg;     //de/potenziamento alla statistica del nemico
} skill;
typedef struct item{
	uchar id;
	char* name;
	char* desc;
	ushort num; // quantita' posseduta
	ushort price;
	ushort sell_price;
	
	// aumento delle statistiche (da sommare a quelle del giocatore)
	int hp;
	int def;
	int dmg;
	float dodge_chance;
	float crit_chance;
	float crit_dmg;
} item;

typedef struct command{
	char up;
	char down;
	char select;
} command;

typedef struct player {
	char* name;
	ushort lv;
	ushort xp;
	int hp; //punti salute
	int hp_max; //punti salute massimi
	int def;
	int ehp; //punti salute emotivi / autostima
	int ehp_max;
	int dmg; //attacco
	float dodge_chance; //probabilita' in %
	float crit_chance;
	float crit_dmg; //moltiplicatore del danno di un colpo critico
	char weapon; //ID dell'arma equipaggiata
	char armor; //ID dell'armatura equipaggiata
	uint money;
} player;

typedef struct enemy {
	uchar id;
	char* name;
	char* desc; //descrizione appare dopo che diventa amico
	char* conv[ENEMY_DIALOGUES]; //testo delle conversazioni
	ushort lv; //livello aumenta con l'aumentare dei soldi del giocatore (per disincentivare l'avidita')
	int hp;
	int hp_max;
	int def;
	int ehp;
	int ehp_max;
	int dmg;
	int edmg; //danno emotivo (inflitto direttamente, al contrario del giocatore che deve usare le abilita')
	float dodge_chance;
	float crit_chance;
	float crit_dmg;
	uchar estat; //stato emotivo
	ushort money;
	bool is_friend; //se diventa amico del giocatore non appare piu' nelle battaglie. se il giocatore fa amicizia con tutti i nemici il gioco finisce, altrimenti può continuare a fare battaglie
} enemy;
/*
	stato emotivo:
	'n' = neutral
	's' = sad
	'a' = angry
	'h' = happy
	'i' = insecure
	'c' = confused
	'y' = hallucinating
*/

char* append(char*,char*,int);
char** str_arr_fill(int,char*[]);

void present_story();
void pausetxt();
void print_text(char*);
int scan_space(char*);

int string_size(char*);

int menu_choice(menu);

void action(int);
void in_inventory();
void in_combat();
void win_screen();
void end_screen();

void skill_print_stats(skill,char);
void skill_upgrade_stats(skill*);

void print_enemy_stats(enemy);
void upgrade_enemy(enemy*,int);
void upgrade_player();

command cmd = {
	.up = 'w',
	.down = 's',
	.select = 'z',
};

player plr = {
	.lv = 1,
	.xp = 0,
	.hp = PLAYER_START_HP,
	.hp_max = PLAYER_START_HP,
	.def = PLAYER_DEF,
	.ehp = PLAYER_START_EHP,
	.ehp_max = PLAYER_START_EHP,
	.dodge_chance = PLAYER_DODGE,
	.dmg = PLAYER_START_DMG,
	.crit_chance = PLAYER_CRIT_CH,
	.crit_dmg = PLAYER_START_CRIT_DMG,
	.weapon = -1,
	.armor = -1,
	.money = 0,
};
menu yes_no_menu;

weapon* weapons;
armor* armors;
skill* skills;
item* items;
enemy* enemies;
enemy curr;
int friends = 0;

int conv_idx; //indica quale conversazione mostrare quando si parla coi nemici

int pos = 0; //posizione
/*
		0 = villaggio
		1 = combattimento
		2 = negozio
		3 = fabbro
		4 = libreria
		5 = psicologo
		6 = uscita dal gioco
*/
	
int main() {
	{ // inizializzare yes/no menu
		yes_no_menu.title = malloc(YES_NO_TITLE_SIZE*sizeof(char));
		yes_no_menu.opt_num = 2;
		char* arr[] = {"Si","No"};
		yes_no_menu.options = str_arr_fill(yes_no_menu.opt_num,arr);
	}
	{ // dichiarare armi, armature, abilita' e oggetti; poi creare i nemici
		weapons = (weapon*)calloc(WEAPON_NUM,sizeof(weapon));
		armors = (armor*)calloc(ARMOR_NUM,sizeof(armor));
		skills = (skill*)calloc(SKILL_NUM,sizeof(skill));
		items = (item*)calloc(ITEM_NUM,sizeof(item));
		enemies = (enemy*)calloc(ENEMY_NUM,sizeof(enemy));
		
		int i;
		{ // armi
			i=0;
			weapons[i++] = (weapon){
				.name = "bastone non magico",
				.desc = "dopo averlo raccolto da terra, il negoziante ha cercato di rifilarti un normalissimo bastone per un prezzo spropositato. e ci e' riuscito.\nnon dovevi cedere all'inganno del capitalismo. mi hai deluso...",
				.price = 99,
				.upgr_price = 0,
				.atk = 0,
				.dodge_chance = 0,
				.crit_chance = 0,
				.crit_dmg = 0,
			};
			weapons[i++] = (weapon){
				.name = "martello",
				.desc = "l'immenso potere del fai da te",
				.price = 50,
				.upgr_price = 7,
				.atk = 2,
				.dodge_chance = 3,
				.crit_chance = 3,
				.crit_dmg = 200,
			};
			weapons[i++] = (weapon){
				.name = "arco",
				.desc = "lancia freccie",
				.price = 175,
				.upgr_price = 5,
				.atk = 5,
				.dodge_chance = 10,
				.crit_chance = 5,
				.crit_dmg = 350,
			};
			weapons[i++] = (weapon){
				.name = "spada",
				.desc = "e' tipo un coltello ma grosso",
				.price = 350,
				.upgr_price = 20,
				.atk = 15,
				.dodge_chance = 0.5,
				.crit_chance = 10,
				.crit_dmg = 200,
			};
			weapons[i++] = (weapon){
				.name = "pugnale",
				.desc = "e' praticamente un coltello",
				.price = 225,
				.upgr_price = 10,
				.atk = 10,
				.dodge_chance = 5,
				.crit_chance = 12,
				.crit_dmg = 250,
			};
		}
		{ // armature
			i=0;
			armors[i++] = (armor){
				.name = "armatura di cuoio",
				.desc = "perche' esiste? e' abbastanza inutile.",
				.price = 100,
				.upgr_price = 10,
				.def = 2,
				.dodge_chance = -5,
			};
			armors[i++] = (armor){
				.name = "armatura di bronzo",
				.desc = "armatura in grado di difenderti da un attacco verbale (pubblicita' ingannevole)",
				.price = 250,
				.upgr_price = 15,
				.def = 5.5,
				.dodge_chance = -18,
			};
			armors[i++] = (armor){
				.name = "armatura di ferro",
				.desc = "armatura appena sufficiente a proteggere da un colpo di spada. fa il suo dovere",
				.price = 400,
				.upgr_price = 25,
				.def = 9,
				.dodge_chance = -30,
			};
			armors[i++] = (armor){
				.name = "armatura d'oro",
				.desc = "costa troppo. ne vale la pena? no.\nti senti bello indossandola? assolutamente si",
				.price = 999,
				.upgr_price = 99,
				.def = 3.5,
				.dodge_chance = -60,
			};
		}
		{ // abilita'
			i=0;
			skills[i++] = (skill){
				.name = "insulto",
				.desc = "fra tutte le abilita' psicologiche che avevi a disposizione, proprio questa dovevi scegliere?\nalmeno costa poco (efficace contro i nemici arrabbiati)",
				.price = 10,
				.upgr_price = 5,
				.strong_vs = 'a',
				.special = false,
				.edmg = 10,
			};
			skills[i++] = (skill){
				.name = "manipolazione",
				.desc = "una tecnica poco simpatica proibita dalla Convenzione di Ginevra.\nnon dovresti farne uso (efficace contro i nemici insicuri, se non lo sono li rende insicuri)",
				.price = 300,
				.upgr_price = 50,
				.strong_vs = 'i',
				.special = true, // cambia lo stato emotivo del nemico in "insicuro"
				.ehp = 10,
				.edmg = 15,
				.dodge_chance = -10,
			};
		}
		{ // oggetti
			i=0;
			items[i++] = (item){
				.name = "pozione di cura piccola",
				.desc = "relativamente salutare (cura 15 hp)",
				.price = 5,
				.sell_price = 3,
				.hp = 15,
			};
			items[i++] = (item){
				.name = "pozione di cura media",
				.desc = "ad alto contenuto di calcio (cura 100 hp)",
				.price = 25,
				.sell_price = 18,
				.hp = 100,
			};
			items[i++] = (item){
				.name = "pozione di cura grande",
				.desc = "meglio dell'alcol (cura 500 hp)",
				.price = 100,
				.sell_price = 70,
				.hp = 500,
			};
		}
		{ // nemici
			i=0;
			enemies[i++] = (enemy){
				.name = "Sblobbo",
				.desc = "E' una massa informe ma con l'ansia",
				.conv = {
					"Chiedi a Sblobbo come va\n\nNon gli va di parlarne...",
					"Chiedi a Sblobbo perche' ti sta attaccando\n\nHa bisogno di calmarsi",
					"Ti offri di ascoltare Sblobbo parlare dei suoi problemi\n\nLui inizia a lamentarsi delle piccole cose che lo fanno andare nel panico\nE' contento di sentirsi ascoltato"
				},
				.hp_max = 90,
				.def = 5,
				.ehp_max = 25,
				.dmg = 10,
				.edmg = 5,
				.dodge_chance = 3,
				.crit_chance = 5,
				.crit_dmg = 200,
				.estat = 'i',
				.money = 25,
			};
			enemies[i++] = (enemy){
				.name = "Sblargo",
				.desc = "E' una massa informe ma razzista",
				.conv = {
					"Chiedi a Sblargo come va\n\nInizia a sbraitare",
					"Chiedi a Sblargo perche' ti sta attaccando\n\nNon sopporta gli umani",
					"Proponi a Sblargo di formare un'alleanza e di fare un colpo di stato\n\nNon sembra entusiasta di schierarsi con un umano, ma per sconfiggere l'impero questo ed altro"
				},
				.hp_max = 55,
				.def = 2,
				.ehp_max = 70,
				.dmg = 15,
				.edmg = 20,
				.dodge_chance = 1,
				.crit_chance = 2.5,
				.crit_dmg = 350,
				.estat = 'a',
				.money = 50,
			};
			enemies[i++] = (enemy){
				.name = "Sblingo",
				.desc = "E' una massa informe ma pacifista",
				.conv = {
					"Chiedi a Sblingo come va\n\nInizia a raccontare di quanto gli piaccia il polpettone",
					"Chiedi a Sblingo perche' ti sta attaccando\n\nRisponde che questo e' un allenamento; l'attivita' fisica fa bene al corpo e allo spirito",
					"Proponi a Sblingo di iscriversi in palestra insieme\n\nVi stringete la mano e iniziate a dirigervi verso la palestra"
				},
				.hp_max = 45,
				.def = 15,
				.ehp_max = 100,
				.dmg = 5,
				.edmg = 5,
				.dodge_chance = 10,
				.crit_chance = 1,
				.crit_dmg = 150,
				.estat = 'h',
				.money = 35,
			};
			enemies[i++] = (enemy){
				.name = "Carlo",
				.desc = "E' un impiegato un po' depresso",
				.conv = {
					"Chiedi a Carlo come va\n\nSi lamenta di quanto sia stressante il lavoro",
					"Chiedi a Carlo perche' ti sta attaccando\n\nDice che il suo passatempo e' il pugilato, per scaricare la tensione",
					"Proponi a Carlo le teorie del Socialismo\n\nSembra elettrizzato all'idea di una societa' cosi' meritocratica come quella socialista"
				},
				.hp_max = 75,
				.def = 15,
				.ehp_max = 20,
				.dmg = 10,
				.edmg = 5,
				.dodge_chance = 4,
				.crit_chance = 10,
				.crit_dmg = 250,
				.estat = 's',
				.money = 10,
			};
			enemies[i++] = (enemy){
				.name = "Grugnoz",
				.desc = "E' un essere come tanti",
				.conv = {
					"Chiedi a Grugnoz come va\n\nSi lamenta di quanto sia brutto non avere particolarita'",
					"Chiedi a Grugnoz perche' ti sta attaccando\n\nRisponde che e' quello che fanno tutte le altre creature",
					"Proponi a Grugnoz di programmare un gioco in C.\nE' una cosa che non fa mai nessuno, cosi' potrebbe distinguersi dalla massa\n\nRisponde che sembra davvero noioso, ma ti ringrazia lo stesso di aver parlato con lui"
				},
				.hp_max = 65,
				.def = 5,
				.ehp_max = 65,
				.dmg = 10,
				.edmg = 10,
				.dodge_chance = 5,
				.crit_chance = 3,
				.crit_dmg = 200,
				.estat = 'n',
				.money = 25,
			};
		}
		
		// mettere dati che sono di default (id = i), owned = false e' gia' settato perche' calloc() mette tutto a 0
		for(i=0;i<WEAPON_NUM;i++) weapons[i].id = i;
		for(i=0;i<ARMOR_NUM;i++) armors[i].id = i;
		for(i=0;i<SKILL_NUM;i++) skills[i].id = i;
		for(i=0;i<ITEM_NUM;i++) items[i].id = i;
		for(i=0;i<ENEMY_NUM;i++) {
			enemies[i].id = i;
			enemies[i].lv = 1;
		}
	}
	menu village = {
		.title = "Sei al villaggio, cosa vuoi fare?",
		.opt_num = 7
	};
	{
		char* arr[] = {"combattimento","negozio","fabbro","libreria","psicologo","inventario","esci"};
		village.options = str_arr_fill(village.opt_num,arr);
	}
	menu shop = {
		.title = "Sei nel negozio, cosa vuoi fare?",
		.opt_num = 6
	};
	{
		char* arr[] = {"compra armi","compra armature","compra oggetti","vendi oggetti","inventario","esci"};
		shop.options = str_arr_fill(shop.opt_num,arr);
	}	
	menu blacksmith = {
		.title = "Sei dal fabbro, cosa vuoi fare?",
		.opt_num = 4
	};
	{
		char* arr[] = {"migliora armi","migliora armature","inventario","esci"};
		blacksmith.options = str_arr_fill(blacksmith.opt_num,arr);
	}	
	menu library = {
		.title = "Sei nella libreria, cosa vuoi fare?",
		.opt_num = 4
	};
	{
		char* arr[] = {"acquisisci abilita'","migliora abilita'","inventario","esci"};
		library.options = str_arr_fill(library.opt_num,arr);
	}
	menu psychologist = {
		.title = "Sei dallo psicologo, cosa vuoi fare?",
		.opt_num = 4
	};
	{
		char* arr[] = {"rispristina autostima","aumenta autostima max","inventario","esci"};
		psychologist.options = str_arr_fill(psychologist.opt_num,arr);
	}
	
	srand(time(0));
	present_story();
	while(1) { // in base alla posizione da' delle opzioni
		switch(pos) {
			case 0:
				action(menu_choice(village));
				break;
			case 1:
				in_combat();
				break;
			case 2:
				action(menu_choice(shop));
				break;
			case 3:
				action(menu_choice(blacksmith));
				break;
			case 4:
				action(menu_choice(library));
				break;
			case 5:
				action(menu_choice(psychologist));
				break;
			case 6:
				printf("grazie per aver giocato\n");
				sleep(1);
				usleep(500000);
				return 0;
			default:
				printf("ERRORE AVVENUTO DURANTE L'ESECUZIONE: luogo non esistente");
				pausetxt();
				return 1;
		}
		usleep(1);
	}
	return 0;
}


void action(int act) {
	
	switch(pos) {
		// villaggio
		case 0:
			if(act==6) in_inventory();
			else if(act==7) {
				yes_no_menu.title = "sei sicuro di voler uscire dal gioco?\ni tuoi progressi non verranno salvati";
				int exit_choice = menu_choice(yes_no_menu);
				if(exit_choice==1) pos = 6;
			}
			else pos = act;
			break;
			
		// combattimento
		case 1:
			if(act==4) in_inventory();
			else if(act==5) {
				print_enemy_stats(curr);
				pausetxt();
			}
			else if(act==6) {
				printf("sei scappato...");
				pausetxt();
				pos = 0;
			}
			else{
				// turno giocatore
				if(act==1) {
					char* text = malloc(256*sizeof(char));
					float crit;
					float dodge;
					int dmg;
					
					crit = (float)(rand()%100);
					dodge = (float)(rand()%100);
					dmg = plr.dmg;
					if(plr.weapon >= 0) dmg += weapons[plr.weapon].atk;
					
					if(crit <= plr.crit_chance+weapons[plr.weapon].crit_chance) {
						if(plr.weapon >= 0) dmg *= (plr.crit_dmg*weapons[plr.weapon].crit_dmg/10000);
						else dmg *= plr.crit_dmg/100;
					}
					
					sprintf(text,"%s attacca %s...\n",plr.name, curr.name);
					print_text(text);
					if(dodge <= curr.dodge_chance){
						print_text(curr.name);
						print_text(" schiva e non subisce danni\n");
					}else{
						if(crit <= plr.crit_chance+weapons[plr.weapon].crit_chance) print_text("COLPO CRITICO!!\n");
						
						dmg *= (float)(100 - curr.def)/100;
						sprintf(text,"%s infligge %d danni a %s\n",plr.name, dmg, curr.name);
						print_text(text);
						
						curr.hp -= dmg;
					}
					free(text);
					pausetxt();
				}
				if(act==2) { // usa abilita'
					int owned_num = 0,i,j;
					for(i=0;i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = "seleziona un'abilita' per vederne le statistiche\nAbilita' possedute:",
						.opt_num = owned_num+1,
					};
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					skill* owned_objs = malloc(owned_num*sizeof(skill));
					for(i=0,j=0;j<owned_num && i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_objs[j++] = skills[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					printf("\033c");
					while(true){
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						sprintf(yes_no_menu.title, "vuoi usare questa abilita'?");
						if(menu_choice(yes_no_menu)==1) {
							char* text = malloc(256*sizeof(char));
							int dmg = owned_objs[obj_idx].edmg;
							
							sprintf(text,"%s usa \"%s\" contro %s...\n",plr.name, owned_objs[obj_idx].name, curr.name);
							print_text(text);
							if(curr.estat == owned_objs[obj_idx].strong_vs) {
								dmg *= 2;
								print_text("COLPO CRITICO!!\n");
							}
							sprintf(text,"%s infligge %d danni emotivi a %s\n",plr.name, dmg, curr.name);
							print_text(text);
							if(owned_objs[obj_idx].ehp != 0) {
								plr.ehp += owned_objs[obj_idx].ehp;
								if(plr.ehp > plr.ehp_max) plr.ehp = plr.ehp_max;
								print_text(plr.name);
								print_text(" riottiene un po' di autostima\n");
							}
							if(owned_objs[obj_idx].enm_atk != 0) {
								curr.dmg += owned_objs[obj_idx].enm_atk;
								if(curr.dmg < 1) curr.dmg = 1;
								print_text(curr.name);
								if(owned_objs[obj_idx].enm_atk < 0) print_text(" viene indebolito\n");
								else print_text(" viene rinforzato\n");
							}
							if(owned_objs[obj_idx].dodge_chance != 0) {
								curr.dodge_chance *= (100 + owned_objs[obj_idx].dodge_chance)/100;
								if(curr.dodge_chance < 0) curr.dodge_chance = 0;
								print_text(curr.name);
								if(owned_objs[obj_idx].dodge_chance < 0) print_text(" e' meno agile\n");
								else print_text(" e' piu' agile\n");
							}
							if(owned_objs[obj_idx].crit_chance != 0) {
								curr.crit_chance *= (100 + owned_objs[obj_idx].crit_chance)/100;
								if(curr.crit_chance < 0) curr.crit_chance = 0;
								print_text(curr.name);
								if(owned_objs[obj_idx].crit_chance < 0) print_text(" e' meno preciso\n");
								else print_text(" e' piu' preciso\n");
							}
							if(owned_objs[obj_idx].crit_dmg != 0) {
								curr.crit_dmg += owned_objs[obj_idx].crit_dmg;
								if(curr.crit_dmg < 0) curr.crit_dmg = 0;
								print_text(curr.name);
								if(owned_objs[obj_idx].crit_chance < 0) print_text(" fara' meno danni critici\n");
								else print_text(" fara' piu' danni critici\n");
							}
							if(owned_objs[obj_idx].id == 1 && curr.estat != 'i') {
								curr.estat = 'i';
								print_text(curr.name);
								print_text(" ora e' insicuro\n");
							}
							pausetxt();
							
							curr.ehp -= dmg;
							break;
							free(text);
						}else print_text("non hai usato l'abilita'");
						pausetxt();
					}
					free(owned_objs);
					free(owned_objs_menu.options);
				}
				if(act==3) { // parla
					print_text(curr.conv[conv_idx]);
					conv_idx++;
					pausetxt();
					if(conv_idx >= ENEMY_DIALOGUES){
						print_text("Evvai!\nora siete diventati amici!");
						enemies[curr.id].is_friend = true;
						friends++;
						pos = 0;
						pausetxt();
						if(friends >= ENEMY_NUM) end_screen();
						break;
					}
				}
				
				if(curr.hp <= 0 || curr.ehp <= 0) win_screen();
				else{ //turno nemico
					if(rand()%2){
						char* text = malloc(256*sizeof(char));
						float crit;
						float dodge;
						int dmg;
						float plr_dodge = plr.dodge_chance;
						if(plr.armor != -1) plr_dodge *= 1 + armors[plr.armor].dodge_chance/100;
						if(plr.weapon != -1) plr_dodge *= 1 + weapons[plr.weapon].dodge_chance/100;
						
						float plr_def = plr.def;
						if(plr.armor != -1) plr_def *= 1 + armors[plr.armor].def/100;
						
						crit = (float)(rand()%100);
						dodge = (float)(rand()%100);
						dmg = curr.dmg;
						
						if(crit <= curr.crit_chance) dmg *= curr.crit_dmg/100;
						
						sprintf(text,"%s attacca %s...\n",curr.name, plr.name);
						print_text(text);
						if(dodge <= plr_dodge){
							print_text(plr.name);
							print_text(" schiva e non subisce danni\n");
						}else{
							if(crit <= curr.crit_chance) print_text("COLPO CRITICO!!\n");
							dmg *= (100 - plr_def)/100;
							sprintf(text,"%s infligge %d danni a %s\n",curr.name, dmg, plr.name);
							print_text(text);
							plr.hp -= dmg;
						}
						free(text);
					} else {
						char* text = malloc(256*sizeof(char));
						int dmg = curr.edmg;
						
						sprintf(text,"%s insulta %s...\n",curr.name, plr.name);
						print_text(text);
						sprintf(text,"%s infligge %d danni emotivi a %s\n",curr.name, dmg, plr.name);
						print_text(text);
						
						plr.ehp -= dmg;
						free(text);
					}
					pausetxt();
				}
				
				if(plr.hp <= 0 || plr.ehp <= 0){ // se il giocatore perde
					print_text("Sei stato sconfitto.\nFortunatamente sei stato risparmiato, dopotutto ");
					print_text(curr.name);
					print_text(" non e' mica un mostro.\n");
					if(plr.hp <= 0 && plr.ehp <= 0) print_text("Ma sei comunque ferito, sia fisicamente che emotivamente");
					else if(plr.hp <= 0){
						print_text("Ma le ferite rimangono lo stesso");
						plr.hp = 1;
					}else if(plr.ehp <= 0){
						print_text("Ma la tua autostima rimane bassa");
						plr.ehp = 1;
					}
					pos = 0;
					pausetxt();
				}
			}
			break;
			
		// negozio
		case 2:
			if(act==1) { // comprare armi
					int not_owned_num = 0,i,j;
					// ottenere il numero di amri non possedute
					for(i=0;i<WEAPON_NUM;i++) if(weapons[i].lv == 0) not_owned_num++;
					// settare il menu
					menu not_owned_objs_menu;
					not_owned_objs_menu.opt_num = not_owned_num+1;
					not_owned_objs_menu.title = malloc(100*sizeof(char));
					sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'arma che desideri comprare", plr.money);
					// allocare le opzioni del menu e fare un array di armi non possedute, per mostrarle al giocatore
					not_owned_objs_menu.options = malloc((not_owned_num+1)*sizeof(char*));
					weapon* not_owned_objs = malloc(not_owned_num*sizeof(weapon));
					// copiare le armi nel rispettivo array. j e' l'indice dell'array delle armi non possedute, i e' l'indice dell'array totale
					for(i=0,j=0;j<not_owned_num && i<WEAPON_NUM;i++) if(weapons[i].lv == 0) not_owned_objs[j++] = weapons[i];
					// setta le opzioni per il menu
					char* price = malloc(10*sizeof(char));
					for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
						// per ogni arma il prezzo e' aggiunto al nome, per mostrare entrambi
						sprintf(price,"%d",not_owned_objs[i].price);
						if(not_owned_objs[i].id == 0) not_owned_objs_menu.options[i] = append(FAKE_NAME,price,30);
						else not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
					}
					not_owned_objs_menu.options[not_owned_num] = "indietro"; // l'ultima opzione e' quella di tornare indietro
					while(true){
						if(not_owned_num>0) {
							if(not_owned_objs[0].id == 0) {
								not_owned_objs[0].name = FAKE_NAME;
								not_owned_objs[0].desc = "OFFERTONA!!! la miglior bacchetta dell'impero a soli 99 soldi!!!\n(dopo averlo raccolto da terra, il negoziante cerca di rifilarti un normalissimo bastone per un prezzo spropositato. non cedere all'inganno del capitalismo)";
								not_owned_objs[0].atk = 999;
								not_owned_objs[0].dodge_chance = 100;
								not_owned_objs[0].crit_chance = 100;
								not_owned_objs[0].crit_dmg = 999;
							}
						}
						int obj_idx = menu_choice(not_owned_objs_menu);
						// se e' l'opzione di tornare indietro
						if(--obj_idx == not_owned_num) break;
						//compra se il giocatore ha abbastanza soldi
						if(plr.money >= not_owned_objs[obj_idx].price) {
							sprintf(yes_no_menu.title, "comprerai: %s\nper %d soldi\ndescrizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\nattacco: %30d\nbonus probabilita' di schivata: %7g%%\nbonus probabilita' di critico: %8g%%\nmoltiplicatore danno critico: %9g%%\n\nsei sicuro?",not_owned_objs[obj_idx].name, not_owned_objs[obj_idx].price,not_owned_objs[obj_idx].desc,not_owned_objs[obj_idx].atk,not_owned_objs[obj_idx].dodge_chance,not_owned_objs[obj_idx].crit_chance,not_owned_objs[obj_idx].crit_dmg);
							printf("\033c");
							// chiedere conferma
							if(menu_choice(yes_no_menu) == 1) {
								plr.money -= not_owned_objs[obj_idx].price;
								// prendere l'ID dell'arma scelta per trovare la corrispondenza nell' array totale delle armi, poi ne cambia il livello
								weapons[not_owned_objs[obj_idx].id].lv = 1;
								printf("hai comprato: %s\n",not_owned_objs[obj_idx].name);
								{ // ricalcolo delle armi non possedute
									not_owned_num--;
									not_owned_objs_menu.opt_num = not_owned_num+1;
									not_owned_objs_menu.options = realloc(not_owned_objs_menu.options,(not_owned_num+1)*sizeof(char*));
									not_owned_objs = realloc(not_owned_objs,not_owned_num*sizeof(weapon));
									for(i=0,j=0;j<not_owned_num && i<WEAPON_NUM;i++) if(weapons[i].lv == 0) not_owned_objs[j++] = weapons[i];
									price = realloc(price,10*sizeof(char));
									sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'arma che desideri comprare", plr.money);
									for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
										sprintf(price,"%d",not_owned_objs[i].price);
										not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
									}
									not_owned_objs_menu.options[not_owned_num] = "indietro";
								}
							}else printf("non hai comprato niente");
						}else printf("comprerai: %s\nper %d soldi\ndescrizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\nattacco: %30d\nbonus probabilita' di schivata: %7g%%\nbonus probabilita' di critico: %8g%%\nmoltiplicatore danno critico: %9g%%\n\nnon hai abbastanza soldi per comprare quest'arma",not_owned_objs[obj_idx].name, not_owned_objs[obj_idx].price,not_owned_objs[obj_idx].desc,not_owned_objs[obj_idx].atk,not_owned_objs[obj_idx].dodge_chance,not_owned_objs[obj_idx].crit_chance,not_owned_objs[obj_idx].crit_dmg);
						pausetxt();
						printf("\033c");
					}
					// liberare la memoria
					free(price);
					free(not_owned_objs);
					free(not_owned_objs_menu.options);
					free(not_owned_objs_menu.title);
					break;
			}
			if(act==2) { // comprare armature
					int not_owned_num = 0,i,j;
					for(i=0;i<ARMOR_NUM;i++) if(armors[i].lv == 0) not_owned_num++;
					menu not_owned_objs_menu;
					not_owned_objs_menu.opt_num = not_owned_num+1;
					not_owned_objs_menu.title = malloc(100*sizeof(char));
					sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'armatura che desideri comprare", plr.money);
					not_owned_objs_menu.options = malloc((not_owned_num+1)*sizeof(char*));
					armor* not_owned_objs = malloc(not_owned_num*sizeof(armor));
					for(i=0,j=0;j<not_owned_num && i<ARMOR_NUM;i++) if(armors[i].lv == 0) not_owned_objs[j++] = armors[i];
					char* price = malloc(10*sizeof(char));
					for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
						sprintf(price,"%d",not_owned_objs[i].price);
						not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
					}
					not_owned_objs_menu.options[not_owned_num] = "indietro";
					while(true){
						int obj_idx = menu_choice(not_owned_objs_menu);
						if(--obj_idx == not_owned_num) break;
						printf("\033c");
						if(plr.money >= not_owned_objs[obj_idx].price) {
							sprintf(yes_no_menu.title, "comprerai: %s\nper %d soldi\ndescizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\ndifesa: %30g%%\nriduzione probab. di schivata: %7g%%\n\nsei sicuro?",not_owned_objs[obj_idx].name,not_owned_objs[obj_idx].price,not_owned_objs[obj_idx].desc,not_owned_objs[obj_idx].def,not_owned_objs[obj_idx].dodge_chance);
							if(menu_choice(yes_no_menu) == 1) {
								plr.money -= not_owned_objs[obj_idx].price;
								armors[not_owned_objs[obj_idx].id].lv = 1;
								printf("hai comprato: %s\n",not_owned_objs[obj_idx].name);
								{
									not_owned_num--;
									not_owned_objs_menu.opt_num = not_owned_num+1;
									not_owned_objs_menu.options = realloc(not_owned_objs_menu.options,(not_owned_num+1)*sizeof(char*));
									not_owned_objs = realloc(not_owned_objs,not_owned_num*sizeof(armor));
									for(i=0,j=0;j<not_owned_num && i<ARMOR_NUM;i++) if(armors[i].lv == 0) not_owned_objs[j++] = armors[i];
									price = malloc(10*sizeof(char));
									sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'armatura che desideri comprare", plr.money);
									for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
										sprintf(price,"%d",not_owned_objs[i].price);
										not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
									}
									not_owned_objs_menu.options[not_owned_num] = "indietro";
								}
							}else printf("non comprato niente");
						}else printf("comprerai: %s\nper %d soldi\ndescizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\ndifesa: %30g%%\nriduzione probab. di schivata: %7g%%\n\nnon hai abbastanza soldi",not_owned_objs[obj_idx].name,not_owned_objs[obj_idx].price,not_owned_objs[obj_idx].desc,not_owned_objs[obj_idx].def,not_owned_objs[obj_idx].dodge_chance);
						pausetxt();
						printf("\033c");
					}
					free(price);
					free(not_owned_objs);
					free(not_owned_objs_menu.options);
					free(not_owned_objs_menu.title);
					break;
			}
			if(act==3) { // comprare oggetti
					int i,j;
					menu objs_menu;
					objs_menu.opt_num = ITEM_NUM+1;
					objs_menu.title = malloc(100*sizeof(char));
					sprintf(objs_menu.title, "hai %d soldi\nScegli dalla lista l'oggetto che desideri comprare", plr.money);
					objs_menu.options = malloc((ITEM_NUM+1)*sizeof(char*));
					char* price = malloc(10*sizeof(char));
					for(i=0;i<ITEM_NUM;i++) {
						sprintf(price,"%d",items[i].price);
						objs_menu.options[i] = append(items[i].name,price,30);
					}
					objs_menu.options[ITEM_NUM] = "indietro";
					while(true){
						int obj_idx = menu_choice(objs_menu);
						if(--obj_idx == ITEM_NUM) break;
						printf("\033c");
						if(plr.money >= items[obj_idx].price) {
							sprintf(yes_no_menu.title, "comprerai: %s\nper %d soldi\ndescizione: %s\n\nsei sicuro?",items[obj_idx].name,items[obj_idx].price,items[obj_idx].desc);
							if(menu_choice(yes_no_menu) == 1) {
								plr.money -= items[obj_idx].price;
								items[obj_idx].num++;
								printf("hai comprato: %s\n",items[obj_idx].name);
								sprintf(objs_menu.title, "hai %d soldi\nScegli dalla lista l'oggetto che desideri comprare", plr.money);
							}else printf("non comprato niente");
						}else printf("comprerai: %s\nper %d soldi\ndescizione: %s\n\nnon hai abbastanza soldi",items[obj_idx].name,items[obj_idx].price,items[obj_idx].desc);
						pausetxt();
						printf("\033c");
					}
					free(price);
					free(objs_menu.options);
					free(objs_menu.title);
					break;
			}
			if(act==4) { // vendere oggetti
					int owned_num = 0,i,j;
					for(i=0;i<ITEM_NUM;i++) if(items[i].num != 0) owned_num++;
					menu owned_objs_menu = {
						.title = malloc(100*sizeof(char)),
						.opt_num = owned_num+1,
					};
					sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un oggetto per potenziarlo\nOggetti posseduti:", plr.money);
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					item* owned_objs = malloc(owned_num*sizeof(item));
					for(i=0,j=0;j<owned_num && i<ITEM_NUM;i++) if(items[i].num != 0) owned_objs[j++] = items[i];
					char* price = malloc(10*sizeof(char));
					for(i=0;i<owned_num;i++) {
						sprintf(price,"+%d",owned_objs[i].sell_price);
						owned_objs_menu.options[i] = append(owned_objs[i].name,price,30);
					}
					for(i=0;i<owned_num;i++) if(owned_objs[i].num != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					while(true){
						printf("\033c");
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						sprintf(yes_no_menu.title,"\n%s\n%s\n\nposseduti: %30d -> %d\n\nsoldi: %32d -> %d\n\nvuoi vedere questo oggetto per %d soldi?\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].num,owned_objs[obj_idx].num-1,plr.money,plr.money+owned_objs[obj_idx].sell_price,owned_objs[obj_idx].sell_price);
						if(menu_choice(yes_no_menu) == 1) {
							owned_objs[obj_idx].num = --items[owned_objs[obj_idx].id].num;
							if(owned_objs[obj_idx].num == 0) {
								owned_num--;
								owned_objs_menu.opt_num = owned_num+1;
								owned_objs_menu.options = realloc(owned_objs_menu.options,(owned_num+1)*sizeof(char*));
								for(i=0,j=0;j<owned_num && i<ITEM_NUM;i++) if(items[i].num != 0) owned_objs[j++] = items[i];
								for(i=0;i<owned_num;i++) {
									sprintf(price,"+%d",owned_objs[i].sell_price);
									owned_objs_menu.options[i] = append(owned_objs[i].name,price,30);
								}
								for(i=0;i<owned_num;i++) if(owned_objs[i].num != 0) owned_objs_menu.options[i] = owned_objs[i].name;
								owned_objs_menu.options[owned_num] = "indietro";
							}
							plr.money += owned_objs[obj_idx].sell_price;
							sprintf(yes_no_menu.title,"\n%s\n%s\n\nposseduti: %30d -> %d\n\nsoldi: %32d -> %d\n\nvuoi vedere questo oggetto per %d soldi?\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].num,owned_objs[obj_idx].num-1,plr.money,plr.money+owned_objs[obj_idx].sell_price,owned_objs[obj_idx].sell_price);
							printf("hai venduto l'oggetto");
						}else printf("non hai venduto l'oggetto");
						pausetxt();
					}
					printf("\033c");
					free(price);
					free(owned_objs);
					free(owned_objs_menu.options);
					free(owned_objs_menu.title);
			}
			if(act==5) in_inventory();
			if(act==6) pos = 0;
			break;
			
		// fabbro
		case 3:
			if(act==1) { //potenziare armi
					int owned_num = 0,i,j;
					// ottenere il numero di armi possedute
					for(i=0;i<WEAPON_NUM;i++) if(weapons[i].lv != 0) owned_num++;
					// setta il menu
					menu owned_objs_menu = {
						.title = malloc(100*sizeof(char)),
						.opt_num = owned_num+1,
					};
					sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'arma per potenziarla\nArmi possedute:", plr.money);
					// allocare le opzioni del menu e fare un array di armi possedute, per mostrarle al giocatore
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					weapon* owned_objs = malloc(owned_num*sizeof(weapon));
					for(i=0,j=0;j<owned_num && i<WEAPON_NUM;i++) if(weapons[i].lv != 0) owned_objs[j++] = weapons[i];
					// copiare le armi nel rispettivo array. j e' l'indice dell'array delle armi non possedute, i e' l'indice dell'array totale
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					while(true){
						printf("\033c"); //svuotare il terminale
						int obj_idx = menu_choice(owned_objs_menu); // ottenere la scelta
						// se e' l'opzione per tornare indietro
						if(--obj_idx == owned_num) break;
						if(owned_objs[obj_idx].id == 0) { // se e' il bastone
							printf("mi prendi in giro?");
							pausetxt();
							continue;
						}
						// stampare le statistiche prima e dopo l'upgrade
						if(plr.money >= owned_objs[obj_idx].upgr_price) {
							sprintf(yes_no_menu.title,"\n%s\n%s\n\nlivello: %30d  -> %d\nattacco: %30d  -> %d\nbonus probabilita' di schivata: %7.3f%% -> %.3f%%\nbonus probabilita' di critico: %8.3f%% -> %.3f%%\nmoltiplicatore danno critico: %9.3f%% -> %.3f%%\n\nsoldi: %32d -> %d\n\nvuoi potenziare quest'arma al prezzo di %d soldi?\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].lv+1,owned_objs[obj_idx].atk,owned_objs[obj_idx].atk+1,owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx].dodge_chance+PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]),owned_objs[obj_idx].crit_chance,owned_objs[obj_idx].crit_chance+PERCENT_FORMULA(owned_objs[obj_idx].crit_chance,owned_objs[obj_idx]),owned_objs[obj_idx].crit_dmg,owned_objs[obj_idx].crit_dmg*1.05,plr.money,plr.money-owned_objs[obj_idx].upgr_price,owned_objs[obj_idx].upgr_price);
							printf("\033c");
							if(menu_choice(yes_no_menu) == 1) {
								owned_objs[obj_idx].atk = weapons[owned_objs[obj_idx].id].atk += 1;
								owned_objs[obj_idx].dodge_chance = weapons[owned_objs[obj_idx].id].dodge_chance += PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]);
								owned_objs[obj_idx].crit_chance = weapons[owned_objs[obj_idx].id].crit_chance += PERCENT_FORMULA(owned_objs[obj_idx].crit_chance,owned_objs[obj_idx]);
								owned_objs[obj_idx].crit_dmg = weapons[owned_objs[obj_idx].id].crit_dmg *= 1.05;
								owned_objs[obj_idx].lv++;
								plr.money -= owned_objs[obj_idx].upgr_price; 
								sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'arma per potenziarla\nArmi possedute:", plr.money);
								printf("%s e' ora a livello %d\n",owned_objs[obj_idx].name,++(weapons[owned_objs[obj_idx].id].lv));
							}else printf("non hai potenziato l'arma");
						}else printf("\n%s\n%s\n\nlivello: %30d  -> %d\nattacco: %30d  -> %d\nbonus probabilita' di schivata: %7.3f%% -> %.3f%%\nbonus probabilita' di critico: %8.3f%% -> %.3f%%\nmoltiplicatore danno critico: %9.3f%% -> %.3f%%\n\nnon hai abbastanza soldi per potenziare quest'arma\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].lv+1,owned_objs[obj_idx].atk,owned_objs[obj_idx].atk+1,owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx].dodge_chance+PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]),owned_objs[obj_idx].crit_chance,owned_objs[obj_idx].crit_chance+PERCENT_FORMULA(owned_objs[obj_idx].crit_chance,owned_objs[obj_idx]),owned_objs[obj_idx].crit_dmg,owned_objs[obj_idx].crit_dmg*1.05);
						pausetxt();
					}
					printf("\033c");
					// libera la memoria
					free(owned_objs);
					free(owned_objs_menu.options);
					free(owned_objs_menu.title);
			}
			if(act==2) { //potenziare le armature
					int owned_num = 0,i,j;
					for(i=0;i<ARMOR_NUM;i++) if(armors[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = malloc(100*sizeof(char)),
						.opt_num = owned_num+1,
					};
					sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'armatura per potenziarla\nArmature possedute:", plr.money);
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					armor* owned_objs = malloc(owned_num*sizeof(armor));
					for(i=0,j=0;j<owned_num && i<ARMOR_NUM;i++) if(armors[i].lv != 0) owned_objs[j++] = armors[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					while(true){
						printf("\033c");
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						if(plr.money >= owned_objs[obj_idx].upgr_price) {
							sprintf(yes_no_menu.title,"\n%s\n%s\n\nlivello: %30d -> %d\ndifesa: %30.3f%% -> %.3f%%\nriduzione probab. di schivata: %7.3f%% -> %.3f%%\n\nsoldi: %32d -> %d\n\nvuoi potenziare quest'armatura al prezzo di %d soldi?\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].lv+1,owned_objs[obj_idx].def,owned_objs[obj_idx].def+PERCENT_FORMULA(owned_objs[obj_idx].def,owned_objs[obj_idx]),owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx].dodge_chance-2*PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]),plr.money,plr.money-owned_objs[obj_idx].upgr_price,owned_objs[obj_idx].upgr_price);
							printf("\033c");
							if(menu_choice(yes_no_menu) == 1) {
								owned_objs[obj_idx].def = armors[owned_objs[obj_idx].id].def += PERCENT_FORMULA(owned_objs[obj_idx].def,owned_objs[obj_idx]);
								owned_objs[obj_idx].dodge_chance = armors[owned_objs[obj_idx].id].dodge_chance -= PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]);
								owned_objs[obj_idx].lv++;
								plr.money -= owned_objs[obj_idx].upgr_price; 
								sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'armatura per potenziarla\nArmature possedute:", plr.money);
								printf("%s e' ora a livello %d\n",owned_objs[obj_idx].name,++(armors[owned_objs[obj_idx].id].lv));
							}else printf("non hai potenziato l'armatura");
						}else printf("\n%s\n%s\n\nlivello: %30d -> %d\ndifesa: %30.3f%% -> %.3f%%\nriduzione probab. di schivata: %7.3f%% -> %.3f%%\n\nnon hai abbastanza soldi per potenziare quest'armatura\n",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].lv+1,owned_objs[obj_idx].def,owned_objs[obj_idx].def+PERCENT_FORMULA(owned_objs[obj_idx].def,owned_objs[obj_idx]),owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx].dodge_chance-2*PERCENT_FORMULA(owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx]));
						pausetxt();
					}
					printf("\033c");
					free(owned_objs);
					free(owned_objs_menu.options);
					free(owned_objs_menu.title);
			}
			if(act==3) in_inventory();
			if(act==4) pos = 0;
			break;
			
		// libreria
		case 4:
			if(act==1) { // comprare abilita'
					int not_owned_num = 0,i,j;
					for(i=0;i<SKILL_NUM;i++) if(skills[i].lv == 0) not_owned_num++;
					menu not_owned_objs_menu;
					not_owned_objs_menu.opt_num = not_owned_num+1;
					not_owned_objs_menu.title = malloc(100*sizeof(char));
					sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'armatura che desideri comprare", plr.money);
					not_owned_objs_menu.options = malloc((not_owned_num+1)*sizeof(char*));
					skill* not_owned_objs = malloc(not_owned_num*sizeof(skill));
					for(i=0,j=0;j<not_owned_num && i<SKILL_NUM;i++) if(skills[i].lv == 0) not_owned_objs[j++] = skills[i];
					char* price = malloc(10*sizeof(char));
					for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
						sprintf(price,"%d",not_owned_objs[i].price);
						not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
					}
					not_owned_objs_menu.options[not_owned_num] = "indietro";
					while(true){
						int obj_idx = menu_choice(not_owned_objs_menu);
						if(--obj_idx == not_owned_num) break;
						printf("\033c");
						skill_print_stats(not_owned_objs[obj_idx],'b');
						if(plr.money >= not_owned_objs[obj_idx].price) {
							if(menu_choice(yes_no_menu) == 1) {
								plr.money -= not_owned_objs[obj_idx].price;
								skills[not_owned_objs[obj_idx].id].lv = 1;
								printf("hai comprato: %s\n",not_owned_objs[obj_idx].name);
								{
									not_owned_num--;
									not_owned_objs_menu.opt_num = not_owned_num+1;
									not_owned_objs_menu.options = realloc(not_owned_objs_menu.options,(not_owned_num+1)*sizeof(char*));
									not_owned_objs = realloc(not_owned_objs,not_owned_num*sizeof(skill));
									for(i=0,j=0;j<not_owned_num && i<SKILL_NUM;i++) if(skills[i].lv == 0) not_owned_objs[j++] = skills[i];
									price = malloc(10*sizeof(char));
									sprintf(not_owned_objs_menu.title, "hai %d soldi\nScegli dalla lista l'abilita' che desideri comprare", plr.money);
									for(i=0;i<not_owned_num;i++) if(not_owned_objs[i].lv == 0) {
										sprintf(price,"%d",not_owned_objs[i].price);
										not_owned_objs_menu.options[i] = append(not_owned_objs[i].name,price,30);
									}
									not_owned_objs_menu.options[not_owned_num] = "indietro";
								}
							}else printf("non comprato niente");
						}
						pausetxt();
						printf("\033c");
					}
					free(price);
					free(not_owned_objs);
					free(not_owned_objs_menu.options);
					free(not_owned_objs_menu.title);
					break;
			}
			if(act==2) { //potenziare abilita'
					int owned_num = 0,i,j;
					for(i=0;i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = malloc(100*sizeof(char)),
						.opt_num = owned_num+1,
					};
					sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'abilita' per potenziarla\nAbilita' possedute:", plr.money);
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					skill* owned_objs = malloc(owned_num*sizeof(skill));
					for(i=0,j=0;j<owned_num && i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_objs[j++] = skills[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					while(true){
						printf("\033c");
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						skill_print_stats(owned_objs[obj_idx],'u');
						if(plr.money >= owned_objs[obj_idx].upgr_price) {
							printf("\033c");
							if(menu_choice(yes_no_menu) == 1) {
								skill_upgrade_stats(&(owned_objs[obj_idx]));
								sprintf(owned_objs_menu.title, "hai %d soldi\nSeleziona un'abilita' per potenziarla\nAbilita' possedute:", plr.money);
								printf("%s e' ora a livello %d\n",owned_objs[obj_idx].name,++(skills[owned_objs[obj_idx].id].lv));
							}else printf("non hai potenziato l'abilita'");
						}
						pausetxt();
					}
					printf("\033c");
					free(owned_objs);
					free(owned_objs_menu.options);
					free(owned_objs_menu.title);
			}
			if(act==3) in_inventory();
			if(act==4) pos = 0;
			break;
			
		// psicologo
		case 5:
			if(act==1) {
				if(plr.ehp == plr.ehp_max) {
					print_text("la tua autostima e' gia' al massimo");
					pausetxt();
					break;
				}else if(plr.money >= 30){
					sprintf(yes_no_menu.title,"seduta dallo psicologo\ncosto: 30 soldi\n\naccetti?");
					if(menu_choice(yes_no_menu) == 1) {
						print_text("autostima ripristinata!\n");
						plr.ehp = plr.ehp_max;
						plr.money -= 30;
					} else print_text("la tua autostima e' rimasta uguale\n");
				}else print_text("torna quando sarai un po' piu' ricco (servono almeno 30 soldi)\n");
				
				pausetxt();
			}
			if(act==2) {
				if(plr.money >= 100){
					sprintf(yes_no_menu.title,"il tuo psicologo vuole insegnarti una tecnica per non cedere allo sconforto\ncosto: 100 soldi\n\naccetti?");
					if(menu_choice(yes_no_menu) == 1) {
						print_text("autostima massima aumentata!\n");
						plr.ehp_max += PLAYER_INCREASE_EHP;
						plr.money -= 100;
					} else print_text("la tua autostima e' rimasta uguale\n");
				}else print_text("torna quando sarai un po' piu' ricco (servono almeno 100 soldi)\n");
				
				pausetxt();
			}
			if(act==3) in_inventory();
			if(act==4) pos = 0;
			break;
	}
}

void in_inventory() {
	menu inventory = {
		.title = "Inventario:",
		.opt_num = 6,
	};
	{
		char* arr[] = {"armi","armature","abilita'","oggetti","info","esci"};
		inventory.options = str_arr_fill(inventory.opt_num,arr);
	}
	int i,a;
	while(true) {
		a = menu_choice(inventory);
		switch(a) {
			case 1: { // mostrare le armi
					int owned_num = 0,i,j;
					for(i=0;i<WEAPON_NUM;i++) if(weapons[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = "seleziona un'arma per vederne le statistiche\nArmi possedute:",
						.opt_num = owned_num+1,
					};
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					weapon* owned_objs = malloc(owned_num*sizeof(weapon));
					for(i=0,j=0;j<owned_num && i<WEAPON_NUM;i++) if(weapons[i].lv != 0) owned_objs[j++] = weapons[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					printf("\033c");
					while(true){
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						sprintf(yes_no_menu.title,"\n%s\n%s\n\nlivello: %30d\nattacco: %30d\nbonus probabilita' di schivata: %7.3f%%\nbonus probabilita' di critico: %8.3f%%\nmoltiplicatore danno critico: %9.3f%%\n\n\nvuoi equipaggiare quest'arma?",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].atk,owned_objs[obj_idx].dodge_chance,owned_objs[obj_idx].crit_chance,owned_objs[obj_idx].crit_dmg);
						if(menu_choice(yes_no_menu) == 1) plr.weapon = owned_objs[obj_idx].id;
						printf("\033c");
					}
					free(owned_objs);
					free(owned_objs_menu.options);
				}
				break;
			case 2: { // mostare le armature
					int owned_num = 0,i,j;
					for(i=0;i<ARMOR_NUM;i++) if(armors[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = "seleziona un'armatura per vederne le statistiche\nArmature possedute:",
						.opt_num = owned_num+1,
					};
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					armor* owned_objs = malloc(owned_num*sizeof(armor));
					for(i=0,j=0;j<owned_num && i<ARMOR_NUM;i++) if(armors[i].lv != 0) owned_objs[j++] = armors[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					printf("\033c");
					while(true){
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						sprintf(yes_no_menu.title,"\n%s\n%s\n\nlivello: %34d\ndifesa: %35.3f%%\nriduzione probabilita' di schivata: %7.3f%%\n\n\nvuoi equipaggiare quest'armatura?",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].lv,owned_objs[obj_idx].def,owned_objs[obj_idx].dodge_chance);
						if(menu_choice(yes_no_menu) == 1){
							plr.armor = owned_objs[obj_idx].id;
						}
						printf("\033c");
					}
					free(owned_objs);
					free(owned_objs_menu.options);
				}
				break;
			case 3: { // mostra abilita'
					int owned_num = 0,i,j;
					for(i=0;i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_num++;
					menu owned_objs_menu = {
						.title = "seleziona un'abilita' per vederne le statistiche\nAbilita' possedute:",
						.opt_num = owned_num+1,
					};
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					skill* owned_objs = malloc(owned_num*sizeof(skill));
					for(i=0,j=0;j<owned_num && i<SKILL_NUM;i++) if(skills[i].lv != 0) owned_objs[j++] = skills[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].lv != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					printf("\033c");
					while(true){
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						skill_print_stats(owned_objs[obj_idx],'i');
						pausetxt();
						printf("\033c");
					}
					free(owned_objs);
					free(owned_objs_menu.options);
				}
				break;
			case 4: { // mostra oggetti
					int owned_num = 0,i,j;
					for(i=0;i<ITEM_NUM;i++) if(items[i].num != 0) owned_num++;
					menu owned_objs_menu = {
						.title = "seleziona un oggetto per vederne le statistiche\nOggetti posseduti:",
						.opt_num = owned_num+1,
					};
					owned_objs_menu.options = malloc((owned_num+1)*sizeof(char*));
					item* owned_objs = malloc(owned_num*sizeof(item));
					for(i=0,j=0;j<owned_num && i<ITEM_NUM;i++) if(items[i].num != 0) owned_objs[j++] = items[i];
					for(i=0;i<owned_num;i++) if(owned_objs[i].num != 0) owned_objs_menu.options[i] = owned_objs[i].name;
					owned_objs_menu.options[owned_num] = "indietro";
					printf("\033c");
					while(true){
						int obj_idx = menu_choice(owned_objs_menu);
						if(--obj_idx == owned_num) break;
						sprintf(yes_no_menu.title,"\n%s\n%s\n\nposseduti: %34d\n\nvuoi usare questo oggetto?",owned_objs[obj_idx].name,owned_objs[obj_idx].desc,owned_objs[obj_idx].num);
						if(menu_choice(yes_no_menu) == 1){
							plr.hp += owned_objs[obj_idx].hp;
							if(plr.hp > plr.hp_max) plr.hp = plr.hp_max;
							
							plr.def += owned_objs[obj_idx].def;
							plr.dmg += owned_objs[obj_idx].dmg;
							
							plr.dodge_chance += owned_objs[obj_idx].dodge_chance;
							plr.crit_chance += owned_objs[obj_idx].crit_chance;
							plr.crit_dmg += owned_objs[obj_idx].crit_dmg;
							
							items[owned_objs[obj_idx].id].num--;
							owned_objs[obj_idx].num--;
							printf("oggetto usato");
							if(items[owned_objs[obj_idx].id].num <= 0) {
								pausetxt();
								break;
							}
						}
						pausetxt();
						printf("\033c");
					}
					free(owned_objs);
					free(owned_objs_menu.options);
				}
				break;
			case 5: { // mostra statistiche giocatore
					weapon plr_weapon = {
						.name = "nessuna: vai nel menu 'armi' nell'inventario per equipaggiarne una",
						.dodge_chance = 0,
						.atk = 0,
						.crit_chance = 0,
						.crit_dmg = 100,
					};
					armor plr_armor = {
						.name = "nessuna: vai nel menu 'armature' nell'inventario per equipaggiarne una",
						.def = 0,
						.dodge_chance = 0,
					};
					if(plr.weapon >= 0) plr_weapon = weapons[plr.weapon];
					if(plr.armor >= 0) plr_armor = armors[plr.armor];
					
					char* critdmg = malloc(64*sizeof(char));
					sprintf(critdmg,"x%.1f",plr.crit_dmg/100);
					
					printf("\nStatistiche giocatore: %15s -> %s\n\n\n","base", "con arma/armatura");
					
					printf("Nome:   %30s\n",plr.name);
					printf("Livello:   %27d\n",plr.lv);
					printf("Esperienza:   %24d XP\n",plr.xp);
					printf("Soldi:  %30d soldi\n\n",plr.money);
					
					printf("Vita:   %30d/%d (%.0f%%)\n",plr.hp, plr.hp_max, (float)plr.hp/plr.hp_max*100);
					printf("Autostima:   %25d/%d (%.0f%%)\n",plr.ehp, plr.ehp_max, (float)plr.ehp/plr.ehp_max*100);
					printf("Difesa:   %28.3f%% -> %.3f%%\n",(float)plr.def, (float)plr.def*(1 + plr_armor.def/100));
					printf("Probabilita' schivata:   %13.3f%% -> %.3f%%\n\n",plr.dodge_chance, plr.dodge_chance*(1 + plr_armor.dodge_chance/100)*(1 + plr_weapon.dodge_chance/100));
					
					printf("Danno:   %29d  -> %d\n",plr.dmg, plr.dmg+plr_weapon.atk);
					printf("Probabilita' critico: %16.3f%% -> %.3f%%\n",plr.crit_chance, plr.crit_chance+plr_weapon.crit_chance);
					printf("Danno critico: %23s  -> x%.1f\n\n",critdmg, plr.crit_dmg*plr_weapon.crit_dmg/10000);
					printf("Arma:   %35s%s\n","", plr_weapon.name);
					printf("Armatura:   %31s%s\n\n","", plr_armor.name);
					pausetxt();
				}
				break;
			case 6:
				free(inventory.options);
				return;
			default:
				printf("ERRORE AVVENUTO DURANTE L'ESECUZIONE: luogo non esistente");
				pausetxt();
				exit(1);
		}
	}
	free(inventory.options);
}
void in_combat() {
	int i;
	{ // crea nemico
		int enemy_num = 0, j=0;
		for(i=0;i<ENEMY_NUM;i++) {
			if(enemies[i].is_friend == false) enemy_num++;
		}
		enemy* avail_enemies = malloc(enemy_num*sizeof(enemy));
		for(i=0;i<ENEMY_NUM;i++) {
			if(enemies[i].is_friend == false){
				avail_enemies[j++] = enemies[i];
			}
		}
		curr = avail_enemies[rand()%enemy_num];
		upgrade_enemy(&curr, pow(plr.money,(float)1/3) * 2);
		conv_idx = 0; // ogni volta che si inizia a combattere un nemico, si parte dal primo dialogo
		
		free(avail_enemies);
	}
	menu combat = {
		.title = "Sei in un combattimento, cosa farai?",
		.opt_num = 6
	};
	{
		char* arr[] = {"attacca","abilita'","parla","inventario","analizza il nemico","scappa"};
		combat.options = str_arr_fill(combat.opt_num,arr);
	}
	 
	print_text("Inizi uno scontro con ");
	print_text(curr.name);
	printf("\n\n");
	print_enemy_stats(curr);
	pausetxt();
	
	while(pos == 1) {
		srand(time(0));
		action(menu_choice(combat));
	}
	return;
}
void win_screen(){
	char* text = malloc(256*sizeof(char));
	int prev_lv = plr.lv;
	sprintf(text,"Hai sconfitto %s!\n\nHai ricevuto %d XP\nHai ricevuto %d soldi\n\n",curr.name,curr.money,curr.money);
	print_text(text);
	plr.money += curr.money;
	plr.xp += curr.money;
	plr.lv = 1 + plr.xp/100;
	upgrade_player();
	if(plr.lv > prev_lv) {
		sprintf(text,"Sei salito di livello!\nLivello: %d -> %d\n\n",prev_lv,plr.lv);
		plr.hp = plr.hp_max;
		plr.ehp = plr.ehp_max;
		print_text(text);
	}
	print_text("Ma sei sicuro di stare seguendo la tua missione?\nForse dovresti cercare di comprendere meglio queste creature...");
	pos = 0;
	free(text);
	pausetxt();
}

void end_screen(){
	print_text("E cosi', hai stretto amicizia con varie creature,\nhai contribuito immensamente per la comprensione di questi esseri.\n");
	print_text("Ti senti finalmente soddisfatto, hai completato l'obiettivo della tua vita.\n\nOra puoi tornare ad essere schiavo del Capitalismo");
	pausetxt();
	exit(0);
}
void present_story() {
	int i;
	char* story[] = {
	"Nel lontano Impero di Psyon,\nc'e' un giovane studente di psicologia,\ndeterminato a scoprire i segreti della mente, del ragionamento e del pensiero.\nIl suo nome e' ",
	"Per approfondire i suoi studi,\n",
	" si e' posto l'obiettivo di comprendere la psicologia dei numerosi esseri che abitano l'Impero.\nPer fare cio', pero', sara' necessario sfidarli in combattimento,\nperche' solo cosi' si sentiranno abbastanza a loro agio per parlare.\nSono creature bizzarre."
	};
	print_text(story[0]);
	char* temp = malloc(256*sizeof(char));
	int tempsize = scan_space(temp);
	while(tempsize > MAX_NAME_LENGTH || tempsize < MIN_NAME_LENGTH) {
		printf("\033cNOME NON VALIDO, la lunghezza deve essere compresa fra %d e %d",MIN_NAME_LENGTH,MAX_NAME_LENGTH);
		sleep(4);
		printf("\033c%s",story[0]);
		tempsize = scan_space(temp);
	}
	plr.name = (char*) malloc(tempsize*sizeof(char));
	for(i=0;i<tempsize;i++) plr.name[i] = temp[i];
	free(temp);
	print_text(story[1]);
	print_text(plr.name);
	print_text(story[2]);
	pausetxt();
	printf("-----COMANDI-----\n");
	printf("%c - su\n%c - giu'\n%c - seleziona\n\nquando vedi tre puntini come quelli sotto puoi premere un tasto qualsiasi per continuare",cmd.up,cmd.down,cmd.select);
	pausetxt();
}
int scan_space (char* str) {
	int i = -1;
	do{
		scanf("%c",&str[++i]);
	}while(str[i]!='\n');
	str[i] = '\0';
	return i+1;
}
int string_size(char* str) {
	int i;
	for(i=0;str[i]!='\0';i++) ;
	return i;
}
void print_text(char* text) {
	int i;
	for(i=0;text[i]!='\0';i++) {
		if(text[i]!='\n') {
			printf("%c",text[i]);
			usleep(TEXT_WAIT);
		}else {
			printf("%c",text[i]);
			usleep(NEWLINE_WAIT);
		}
	}
}
// nota: la scelta parte da 1, non da 0
int menu_choice(menu a) {
	int choice = 1,i,flag = 1; // la flag e' per far comparire la scelta solo dopo che un tasto e' premuto. se flag = 2,choice = -1 allora funziona. per disabilitare basta mettere flag = 1,choice = 1
	char c;
	do{
		printf("%s\n",a.title);
		// stampa tutte le opzioni e evidenzia quella su cui si trova il cursore
		for(i=0;i<a.opt_num;i++) {
			if(choice==i+1) printf(">\033[1;32m");
			else printf(" ");
			printf(" %s\033[1;0m\n",a.options[i]);
		}
		// prende l'input e si muove sopra o sotto
		do{
			c = getch();
		}while(((c!=cmd.up && c!=cmd.down) || a.opt_num<2) && c!=cmd.select && flag!=2); // se non viene premuto un tasto valido non si ricarica lo schermo (per effetti visivi)
		// cambia l'opzione scelta e abilita l'effetto pacman
		if(c==cmd.up) {
			if(choice==1) choice=a.opt_num;
			else choice--;
		}
		if(c==cmd.down) {
			if(choice==a.opt_num) choice=1;
			else choice++;
		}
		
		if(flag--==2) choice = 1;
		if(flag!=1) flag = 0;
		printf("\033c");
	}while(c!=cmd.select || flag!=0);
	return choice;
}
void pausetxt() {
	printf("\n\n          ...\n");
	getch();
	printf("\033c");
}
char** str_arr_fill(int size,char* str_arr[]) {// per riempire un char** dato un char*[]. usando un char*[] invece di un char** da' errori
	char** str_ptr = (char**)malloc(size*sizeof(char*));
	int i;
	for(i=0;i<size;i++) str_ptr[i] = str_arr[i];
	return str_ptr;
}
char* append(char* str1, char* str2, int total_size) { //total_size e' per quando si vuole una stringa in output piu' lunga della somma delle altre 2
	int i;
	char* ans;
	if(total_size>(string_size(str1)+string_size(str2))) {
		int space_chars = total_size - (string_size(str1)+string_size(str2)); // the number of empty characters between a string and the other
		ans = (char*)malloc((total_size + 1)*sizeof(char));
		for(i=0; i<string_size(str1); i++) ans[i] = str1[i];
		for(i=0; i<space_chars; i++) ans[i+string_size(str1)] = ' ';
		for(i=0; i<string_size(str2); i++) ans[i+string_size(str1)+space_chars] = str2[i];
		ans[string_size(str1)+string_size(str2)+space_chars] = '\0';
	}
	else {
		ans = (char*)malloc((string_size(str1)+string_size(str2)+1)*sizeof(char));
		for(i=0; i<string_size(str1); i++) ans[i] = str1[i];
		for(i=0; i<string_size(str2); i++) ans[i+string_size(str1)] = str2[i];
		ans[string_size(str1)+string_size(str2)] = '\0';
	}
	return ans;
}

void skill_print_stats(skill obj,char parameter) { // il parametro e' 'b' per buy, 'i' per inventory o 'u' per upgrade
	char* temp_str = calloc(512,sizeof(char));
	
	if(parameter=='b'){
		switch(obj.id){
			case 0:
				sprintf(temp_str, "comprerai: %s\nper %d soldi\ndescrizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\ndanno emotivo: %25d\n\n",obj.name, obj.price,obj.desc,obj.edmg);
				break;
			case 1:
				sprintf(temp_str, "comprerai: %s\nper %d soldi\ndescrizione: %s\n\nstatistiche a livello 1:\n------------------------------------------\ndanno emotivo: %25d\nbonus autostima: %23d\nriduzione schivata nemico: %13g%%\n\n",obj.name, obj.price,obj.desc,obj.edmg,obj.ehp,obj.dodge_chance);
				break;
			default:
				printf("ERRORE: quest'abilita' non esiste.\n");
				exit(6);
				break;
		}
		if(plr.money >= obj.price) {
			temp_str = append(temp_str,"sei sicuro?\n",0);
			sprintf(yes_no_menu.title,"%s",temp_str);
		}else{
			temp_str = append(temp_str,"non hai abbastanza soldi per comprare quest'abilita'\n",0);
			printf("%s",temp_str);
		}
	}
	if(parameter=='i'){
		switch(obj.id) {
			case 0:
				printf("\n%s\n%s\n\nlivello: %31d\ndanno emotivo: %25d\n\n",obj.name,obj.desc,obj.lv,obj.edmg);
				break;
			case 1:
				printf("\n%s\n%s\n\nlivello: %31d\ndanno emotivo: %25d\nbonus autostima: %23d\nriduzione schivata nemico: %13g%%\n\n",obj.name,obj.desc,obj.lv,obj.edmg,obj.ehp,obj.dodge_chance);
				break;
			default:
				printf("ERRORE: quest'abilita' non esiste.\n");
				exit(6);
				break;
		}
	}
	if(parameter=='u'){
		switch(obj.id) {
			case 0:
				sprintf(temp_str,"\n%s\n%s\n\nlivello: %31d -> %d\ndanno emotivo: %25d -> %d\n\n",obj.name,obj.desc,obj.lv,obj.lv+1,obj.edmg,obj.edmg+1);
				break;
			case 1:
				obj.dodge_chance *= -1;
				sprintf(temp_str,"\n%s\n%s\n\nlivello: %31d -> %d\ndanno emotivo: %25d -> %d\nbonus autostima: %23d -> %d\nriduzione schivata nemico: %12.3f%% -> %.3f%%\n\n",obj.name,obj.desc,obj.lv,obj.lv+1,obj.edmg,obj.edmg+1,obj.ehp,obj.ehp+1,-obj.dodge_chance,-(obj.dodge_chance+PERCENT_FORMULA(obj.dodge_chance,obj)));
				obj.dodge_chance *= -1;
				break;
			default:
				printf("ERRORE: quest'abilita' non esiste.\n");
				exit(6);
				break;
		}
		if(plr.money >= obj.upgr_price) {
			sprintf(yes_no_menu.title,"%ssoldi: %33d -> %d\n\nvuoi potenziare quest'abilita' al prezzo di %d soldi?\n",temp_str,plr.money,plr.money-obj.upgr_price,obj.upgr_price);
		}else{
			temp_str = append(temp_str,"non hai abbastanza soldi per potenziare quest'abilita'\n",0);
			printf("%s",temp_str);
		}
	}
	free(temp_str);
	return;
}
void skill_upgrade_stats(skill* obj) {
	switch(obj->id) {
		case 0:
			obj->lv++;
			obj->edmg = skills[obj->id].edmg += 1;
			plr.money -= skills[obj->id].upgr_price;
			break;
		case 1:
			obj->lv++;
			obj->edmg = skills[obj->id].edmg += 1;
			obj->ehp = skills[obj->id].ehp += 1;
			
			skills[obj->id].dodge_chance *= -1;
			skills[obj->id].dodge_chance += PERCENT_FORMULA(skills[obj->id].dodge_chance,skills[obj->id]);
			obj->dodge_chance = skills[obj->id].dodge_chance *= -1;
			
			plr.money -= skills[obj->id].upgr_price;
			break;
		default:
			printf("ERRORE: quest'abilita' non esiste.\n");
			exit(6);
			break;
	}
}
void print_enemy_stats(enemy obj){
	
	char* emotional_status;
	switch(obj.estat){
		case 'n':
			emotional_status = "neutrale";
			break;
		case 's':
			emotional_status = "triste";
			break;
		case 'a':
			emotional_status = "arrabbiato";
			break;
		case 'h':
			emotional_status = "felice";
			break;
		case 'i':
			emotional_status = "insicuro";
			break;
		case 'c':
			emotional_status = "confuso";
			break;
		case 'y':
			emotional_status = "con allucinazioni";
			break;
		default:
			emotional_status = "sconosciuto";
			break;
	}
	printf("descrizione:   %s\n",obj.desc);
	printf("livello:       %12d\n",obj.lv);
	printf("vita:          %12d/%d (%.0f%%)\n",obj.hp,obj.hp_max, (float)obj.hp/obj.hp_max*100);
	printf("autostima:     %12d/%d (%.0f%%)\n",obj.ehp,obj.ehp_max, (float)obj.ehp/obj.ehp_max*100);
	printf("difesa:        %12d%%\n",obj.def);
	printf("danno:         %12d\n",obj.dmg);
	printf("danno emotivo: %12d\n",obj.edmg);
	printf("schivata:      %12.2f%%\n",obj.dodge_chance);
	printf("critico:       %12.2f%%\n",obj.crit_chance);
	printf("danno critico: %12.2f%%\n",obj.crit_dmg);
	printf("stato emotivo:  %12s",emotional_status);
}
void upgrade_enemy(enemy* enm,int level){
	if(level < 1) level = 1;
	for(enm->lv=1; enm->lv < level; enm->lv++){
		enm->def += PERCENT_FORMULA(enm->def,(*enm));
		enm->dodge_chance += PERCENT_FORMULA(enm->dodge_chance,(*enm));
		enm->crit_chance += PERCENT_FORMULA(enm->crit_chance,(*enm));
		enm->crit_dmg += PERCENT_FORMULA(enm->crit_dmg,(*enm));
	}
	enm->dmg *= pow(ENEMY_UPGRADE_FACTOR,level-1);
	enm->edmg *= pow(ENEMY_UPGRADE_FACTOR,level-1);
	enm->hp_max *= pow(ENEMY_UPGRADE_FACTOR,level-1);
	enm->ehp_max *= pow(ENEMY_UPGRADE_FACTOR,level-1);
	
	enm->hp = enm->hp_max;
	enm->ehp = enm->ehp_max;
	
	enm->money *= pow(ENEMY_UPGRADE_FACTOR,level-1);
}
void upgrade_player(){
	int player_lv = plr.lv;
	
	for(plr.lv = 1; plr.lv < player_lv; plr.lv++){
		plr.def += PERCENT_FORMULA(plr.def,plr);
		plr.dodge_chance += PERCENT_FORMULA(plr.dodge_chance,plr);
		plr.crit_chance += PERCENT_FORMULA(plr.crit_chance,plr);
	}
	
	plr.hp_max = PLAYER_START_HP + PLAYER_INCREASE_HP*(plr.lv-1);
	plr.dmg = PLAYER_START_DMG + PLAYER_INCREASE_DMG*(plr.lv-1);
	plr.crit_dmg = PLAYER_START_CRIT_DMG + PLAYER_INCREASE_CRIT_DMG*(plr.lv-1);
}

