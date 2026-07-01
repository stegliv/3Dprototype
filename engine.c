#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * Structures
 */

char screen[192][192];

struct projection {
    float x;
    float y;
} typedef projection;

struct vector {
    float x;
    float y;
    float z;
} typedef vector;

struct triangle {
    vector a;
    vector b;
    vector c;
} typedef triangle;

struct camera {
    vector pos;
    vector direction;
    float angle;
    float fov;
} typedef cam;

/*
 * Fonctions mathématiques
 */

int pent(float x) { // Partie entière
    int result = x;
    return result;
}

float Q_rsqrt( float number ) // Implémentation du fast inverse square root
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;
    i  = 0x5f3759df - ( i >> 1 );
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );
    return y;
}



/*
 * Actions sur les vecteurs
 */

vector nvec(float x, float y, float z) {
    vector result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

vector scale(vector u, float l) {
    vector result;
    result.x = l * u.x;
    result.y = l * u.y;
    result.z = l * u.z;
    return result;
}

vector add(vector u, vector v) {
    vector result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    result.z = u.z + v.z;
    return result;
}

vector sub(vector u, vector v) {
    vector result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    result.z = u.z - v.z;
    return result;
}

float inorm(vector u) { // Donne l'inverse de la norme d'un vecteur
    return Q_rsqrt(u.x*u.x + u.y*u.y + u.z*u.z);
}

vector normalisation(vector u) { // Prend un vecteur, donne le vecteur directeur
    return scale(u, inorm(u));
}


/*
 * Gestion de l'affichage
 */

void n() { // Saut de ligne, évite de retaper printf à chaque fois.
    printf("\n");
}

void scr() { // Screen reset : On supprime tout ce qui est contenu dans l'écran. Sert aussi à initialiser l'écran.
    for (int i = 0; i < 192; i++) {
        for (int j = 0; j < 192; j++) {
            screen[j][i] = '_';
        }
    }
}

void scd() { // Screen display : On affiche ce qui est à l'écran
    for (int i = 1; i < 191; i++) {
        for (int j = 1; j < 191; j++) {
            printf("%c ", screen[j][192 - i]);
        }
        n();
    }
}

void scc() { // Screen clear : On fait plein de sauts de ligne pour préparer le prochain affichage
    for (int i = 0; i<192; i++) {
        n();
    }
}

vector getcoord(vector a, cam c) {
    vector toshow = a;
    vector direction = c.direction;
    vector pxz = normalisation(nvec((c.direction).x, 0, (c.direction).z));

    if (pxz.x != 0 || pxz.z != 0) {
        toshow = nvec((pxz.z) * (toshow.x) - (pxz.x)*toshow.z, toshow.y, (pxz.x) * (toshow.x) + (pxz.z)*(toshow.z));
        direction = nvec( (pxz.z) * (direction.x) - (pxz.x)*direction.z, direction.y, (pxz.x) * (direction.x) + (pxz.z) * (direction.z));

    }
    vector pyz = normalisation(direction);
    if (pyz.y != 0 || pyz.z != 0) {
        toshow = nvec(toshow.x, pyz.z * toshow.y - pyz.y * toshow.z, (pyz.y) * toshow.y + toshow.z * pyz.z);
    }
    return toshow;
}

void vd(vector a, vector b, cam c) {
    vector ta = getcoord(sub(a, c.pos), c);
    vector tb = getcoord(sub(b, c.pos), c);

    float angle = c.angle;
    float fov = ((c.fov/360) * 6.283185)/ 2;
    


    if (tb.z <= 0 && ta.z > 0) {
        vector stockage = ta;
        ta = tb;
        tb = stockage;
    }

    if (ta.z <= 0 && tb.z > 0) {
        vector na = ta;
        vector nb = tb;

        float tan2 = 2 * tan(fov);
        tan2 = tan2 * tan2;
        tan2 = 1/tan2;
        float xa = ta.x;
        float xa2 = xa*xa;
        float ya = ta.y;
        float ya2 = ya*ya;
        float za = ta.z;
        float za2 = za*za;
        float xb = tb.x;
        float xb2 = xb*xb;
        float yb = tb.y;
        float yb2 = yb*yb;
        float zb = tb.z;
        float zb2 = zb*zb;
        float xaxb = xa*xb;
        float yayb = ya*yb;
        float zazb = za*zb;
        float coeffa = za2 - 2*zazb + zb2 - tan2*(xa2 - 2*xaxb + xb2 + ya2 - 2*yayb + yb2);
        float coeffb = 2*(zazb - zb2 - tan2*(xaxb - xb2 + yayb - yb2));
        float coeffc = zb2 - tan2*(xb2 + yb2);

        float delta = 1/(coeffb*coeffb - 4*coeffa*coeffc);
        float t = Q_rsqrt(delta);
        float t1 = (-coeffb - t)/(2*coeffa);

        ta = add(scale(ta, t1), scale(tb, 1-t1));
        

    }

    if (ta.z > 0 && tb.z > 0) {
        ta = scale(ta, 1/(ta.z));
        tb = scale(tb, 1/(tb.z));



        ta = nvec(ta.x * cos(angle) - ta.y * sin(angle), ta.x * sin(angle) + ta.y * cos(angle), 0);
        tb = nvec(tb.x * cos(angle) - tb.y * sin(angle), tb.x * sin(angle) + tb.y * cos(angle), 0);
        

        ta = scale(ta, 96*(1/tan(fov)));
        tb = scale(tb, 96*(1/tan(fov)));


        vector glider = sub(ta, tb);
        float nrm = inorm(glider);
        float ite = pent(1/nrm) + 1;
        glider = normalisation(glider);
        ta = add(ta, nvec(96.5, 96.5, 0));
        tb = add(tb, nvec(96.5, 96.5, 0));
        int X;
        int Y;
        for (int i = 0; i < ite; i++) {
            X = pent(tb.x);
            Y = pent(tb.y);
            if (X > 0 && X < 192 && Y > 0 && Y < 192) {
                screen[X][Y] = '#';
            }
            tb = add(tb, glider);
        }
    }
}

void fisheye(vector a, vector b, cam c) { // Prend deux vecteurs en entrée, dessine le vecteur en fonction d'une caméra

    vector ta = getcoord(sub(a, c.pos), c);
    vector tb = getcoord(sub(b, c.pos), c);


    // On part du principe qu'il n'y a rien sur la caméra

    vector ua = ta;
    vector ub = tb;
    float L;
    if (ta.z < 0 && tb.z >= 0) {
        L = tb.z / (tb.z - ta.z);
        ta = add(scale(ta, L), scale(tb, 1 - L));
    } else if (tb.z < 0 && ta.z >= 0) {
        L = tb.z / (tb.z - ta.z);
        ta = add(scale(ta, L), scale(tb, 1 - L));
    }


    if (tb.z >= 0 && ta.z >= 0) {
        ua.z = 0;
        ub.z = 0;

        float nrm1 = 1/inorm(ua);
        float nrm2 = 1/inorm(ub);
        float nrm3 = 1/inorm(ta);
        float nrm4 = 1/inorm(tb);

        ua = normalisation(ua);
        ub = normalisation(ub);
        ua = scale(ua, (nrm1/nrm3)*136);
        ub = scale(ub, (nrm2/nrm4)*136);
        ua = add(ua, nvec(96.5, 96.5, 0));
        ub = add(ub, nvec(96.5, 96.5, 0));
        // Ici le FOV est fixe.
        // Idée pour régler le problème sans arccos : on fait une rotation, on éloigne le vecteur de l'axe z.

        vector final = sub(ua, ub);

        float inversenorme = inorm(final);
        final = scale(final, inversenorme);
        int ite = pent(1/inversenorme) + 1;
        int X;
        int Y;
        for (int i = 0; i < ite; i++) {
            X = pent(ub.x);
            Y = pent(ub.y);
            if (X > 0 && X < 192 && Y > 0 && Y < 192) {
                screen[X][Y] = '#';
            }
            ub = add(ub, final);
        }
    }
}


vector *sphere(float rayon, float x, float y, float z) {
    vector *result = malloc(288*sizeof(vector));
    vector ref = nvec(rayon, 0, 0);
    result[0] = ref;
    for (int i = 1; i<48; i++) {
        result[i] = nvec(rayon * cos((i * 3.141592) / 24), rayon * sin((i * 3.141592) / 24), 0);
    }
    for (int i = 1; i < 6; i++) {
        for (int j = 0; j < 48; j++) {
            result[48*i + j] = nvec(result[j].x * cos((i * 3.141592)/8) - result[j].z * sin((i * 3.141592)/8), result[j].y, result[j].x * sin((i * 3.141592)/8) + result[j].z * cos((i * 3.141592)/8));
        }
    }
    return result;
}

vector *cube(float cote, float x, float y, float z) {
    vector *result = malloc(8*sizeof(vector));
    float c = cote/2;
    for (int i = 0; i < 8; i++) {
        result[i] = nvec(x + c - 2*((i&4)>>2)*c, y + c - 2*((i&2)>>1)*c, z + c - 2*((i&1)*c));
    }
    return result;
}

void approx(vector a, vector b, int precision, cam c) {
    float precis = precision;
    vector glider = scale(sub(a, b), 1/precis);
    vector source = b;
    vector goal = add(b, glider);
    for (int i = 0; i < precision; i++) {
        vd(goal, source, c);
        source = add(source, glider);
        goal = add(goal, glider);
    }
}

void drawsphere(vector *todraw, cam camera) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 48; j++) {
            vd(todraw[48*i + (j%48)], todraw[48*i + ( (j+1)%48 )], camera);
        }
    }
}

void drawcube(vector *todraw, cam camera) {
    vd(todraw[0], todraw[1], camera);
    vd(todraw[0], todraw[2], camera);
    vd(todraw[2], todraw[3], camera);
    vd(todraw[1], todraw[3], camera);
    vd(todraw[4], todraw[0], camera);
    vd(todraw[5], todraw[1], camera);
    vd(todraw[6], todraw[2], camera);
    vd(todraw[7], todraw[3], camera);
    vd(todraw[4], todraw[5], camera);
    vd(todraw[4], todraw[6], camera);
    vd(todraw[5], todraw[7], camera);
    vd(todraw[6], todraw[7], camera);
    // scd();
}

int main(void) {
    cam camera;
    camera.pos = nvec(30, 30, 60);
    camera.direction = nvec(-1, -1, -2);
    camera.angle = 0;
    camera.fov = 90;

    scr();

    /*
    vector *kube = cube(50, 0, 0, 0);
    drawcube(kube, camera);
    scd();
    */

    /*
    vector *spheru = sphere(3, 0, 0, 0);
    drawsphere(spheru, camera);

    scd();
    */

    
    for (int i = 0; i < 64; i++) {
        vector *kube = cube(10, 10*(i&3), 0, 10*((i&(0b1100))>>2));
        drawcube(kube, camera);
        free(kube);
    }

    vector *kube = cube(10, 20, 10, 20);
    drawcube(kube, camera);
    

    scd();
    

    printf("Fin de l'exécution.");
    n();
    return 0;
}

/*
    vd(todraw[0], todraw[1], camera);
    vd(todraw[0], todraw[2], camera);
    vd(todraw[2], todraw[3], camera);
    vd(todraw[1], todraw[3], camera);
    vd(todraw[4], todraw[0], camera);
    vd(todraw[5], todraw[1], camera);
    vd(todraw[6], todraw[2], camera);
    vd(todraw[7], todraw[3], camera);
    vd(todraw[4], todraw[5], camera);
    vd(todraw[4], todraw[6], camera);
    vd(todraw[5], todraw[7], camera);
    vd(todraw[6], todraw[7], camera);
    */


/*
float cs(float x) {
    int mode = ((pent(x*10000)) / 31415)&1;
    float modulo = ((pent(x * 10000)) % 31415);
    modulo = modulo/10000;
    float result = 1;
    int rhesus = 1;
    float pow = 1;
    int facto = 1;
    for (int i = 2; i < 10; i+=2) {
        pow = pow*modulo*modulo;
        facto = facto*i*(i-1);
        rhesus = rhesus*(-1);
        result = result + rhesus*(pow/facto);
    }
    result = (1 - 2*mode)*result;
    return result;
}

float sn(float x) {
    int mode = ((pent(x*10000)) / 31415)&1;
    float modulo = ((pent(x * 10000)) % 31415);
    modulo = modulo/10000;
    float result = modulo;
    int rhesus = 1;
    float pow = modulo;
    int facto = 1;
    for (int i = 3; i < 11; i+=2) {
        pow = pow*modulo*modulo;
        facto = facto*i*(i-1);
        rhesus = rhesus*(-1);
        result = result + rhesus*(pow/facto);
    }
    result = (1 - 2*mode)*result;
    return result;
}


void sv(vector a, cam c) {
    vector ta = getcoord(sub(a, c.pos), c);

    // On part du principe qu'il n'y a rien sur la caméra

    vector ua = ta;
    printf("%f, %f, %f\n", ua.x, ua.y, ua.z);
    ua.z = 0;
    ua = normalisation(ua);
    printf("%f, %f, %f\n", ua.x, ua.y, ua.z);

    float nrm1 = 1/inorm(ua);
    float nrm3 = 1/inorm(ta);
    // nrm1/nrm3
    ua = scale(ua, (nrm1/nrm3)*136);
    printf("%f, %f, %f\n", ua.x, ua.y, ua.z);
    ua = add(ua, nvec(96.5, 96.5, 0));
    // Ici le FOV est fixe.
    // Idée pour régler le problème sans arccos : on fait une rotation, on éloigne le vecteur de l'axe z.
    int X = pent(ua.x);
    int Y = pent(ua.y);
    screen[X][Y] = '#';
}
    */


    /*
        na.z = 0;
        nb.z = 0;
        nb = normalisation(nb);
        float cosinus1 = nb.x;
        float sinus1 = nb.y;
        na = normalisation(nvec(na.x * cosinus1 + sinus1 * na.y, na.y * cosinus1 - (na.x * sinus1), 0));
        printf("Rotation 1 na = (%f, %f, %f)", na.x, na.y, na.z); n();
        float cosinus2 = na.x;
        float sinus2 = na.y;
        na = nvec(ta.x * cosinus2 + ta.y * sinus2, ta.y * cosinus2 - (ta.x * sinus2), 0);
        printf("Rotation 2 na = (%f, %f, %f)", na.x, na.y, na.z); n();
        // On fait une rotation sur ta ici pour qu'il soit aligné à tb, quand on observe la scène depuis derrière la caméra

        // Maintenant, on fait avancer na jusqu'à ce qu'il soit pile sur le cône du champ de vision. Comme on se limite à 180°, on
        // a déjà évité les problèmes de devoir ramener un point infiniment loin sur un écran.

        float norme = 1/(inorm(na));
        float normenb = 1/(inorm(nb));
        float targetz = (tb.z / normenb) * norme;
        na.z = targetz; // Maintenant, na.z est colinéaire à tb.z.

        norme = 1/(inorm(na));
        norme = norme*norme;
    

        
        // On cherche le projeté orthogonal à notre nouveau vecteur.
        float lambda = (na.x * ta.x + na.y * ta.y + na.z * ta.z)/norme;
        vector movea = normalisation(add(ta, -scale(na, lambda))); // Ce vecteur est orthogonal au cône de vision.
        norme = 1/(inorm(na));
        vector final = scale(normalisation(sub(b, a)), norme); // Changer le nom, mais en gros ce vecteur devrait être au bon endroit

        ta = final;
        printf("ta = (%f, %f, %f)", ta.x, ta.y, ta.z);
        n();
        */