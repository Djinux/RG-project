# Priroda i Drustvo!
Djina Cizmas 125/2018

# Uputstvo
Sva interakcija obavlja se preko ImGui prozora. Oni se aktiviraju pritiskom na F1.
Kretanje je isto kao u project_base-u (w, a, s, d).

# Implementirane oblasti:
1. Svetla na sceni: 
    1. Directional - "Moonlight"
    2. Point Light - "Streetlight"
    3. Spotlight - "Flashlight"
- Svetla je moguce menjati, odnosno odabrati zeljeno cekiranjem jednog od checkbox-eva u ImGui prozoru.
2. Blending:
    1. discard prilikom crtanja modela "Dusty Road" i "Oak Tree"
    2. primenjena blend f-ja prilikom crtanja modela "Plastic Bottle" - i pritom promenjena alpha vrednost flase u shader-u kako bi bila providnija
3. Face culling: Napomena!!! Morao je biti disable-ovan pre crtanja modela "Pile" i enable-ovan nakon njegovog crtanja jer se model nije ponasao ocekivano.

-Oblast iz grupe A: Cubemap - ucitan je skybox (zvezdano nebo)
-Oblast iz grupe B: Normal Mapping and Parallax Mapping - efekti se mogu videti na modelu "Plank." Model je rucno nacrtan i na njega nalepljena tekstura koja je imala dostupnu mapu normala i visina.
U nedostatku modela koji imaju sve potrebne mape, morala sam ovako da se snadjem. 

PS: Definitivno bih volela da doradim projekad za sledeci rok.

1. youtube: https://www.youtube.com/watch?v=NPP9Tg04Imk&ab_channel=arabella2317 
