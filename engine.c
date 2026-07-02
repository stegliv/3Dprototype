#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h> // current version, rendering is done in a proper window, rather than a shell using ascii.

/*
 * Structures
 */

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;


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

    vector planxz;
    vector planyz;

    float angle;
    float fov;
    float true_fov;
    float inv_tan;

    float cosangle;
    float sinangle;

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

void setcamangle(cam *camera, float a) {
    (*camera).angle = a;
    (*camera).cosangle = cos(a);
    (*camera).sinangle = sin(a);
}

void setcamdirection(cam *camera, vector v) {
    (*camera).direction = v;
    (*camera).planxz = normalisation(nvec(v.x, 0, v.z));
    float x = ((*camera).planxz).x;
    float z = ((*camera).planxz).z;
    (*camera).planyz = normalisation(nvec( z * (v.x) - x*(v.z), v.y, x * (v.x) + z * (v.z)));
}

void setcamfov(cam *camera, float a) {
    (*camera).fov = a;
    float newfov = ((a/360) * 6.283185)/ 2;
    (*camera).true_fov = newfov;
    (*camera).inv_tan = 1/(tan(newfov));
}

/*
 * Gestion de l'affichage
 */


vector getcoord(vector a, cam c) {
    vector toshow = a;
    vector direction = c.direction;
    vector pxz = c.planxz;
    vector pyz = c.planyz;

    if (pxz.x != 0 || pxz.z != 0) {
        toshow = nvec((pxz.z) * (toshow.x) - (pxz.x)*toshow.z, toshow.y, (pxz.x) * (toshow.x) + (pxz.z)*(toshow.z));
    }

    if (pyz.y != 0 || pyz.z != 0) {
        toshow = nvec(toshow.x, pyz.z * toshow.y - pyz.y * toshow.z, (pyz.y) * toshow.y + toshow.z * pyz.z);
    }
    return toshow;
}

void vd(vector a, vector b, cam c, SDL_Renderer *d) {
    vector ta = getcoord(sub(a, c.pos), c);
    vector tb = getcoord(sub(b, c.pos), c);

    float cosinus_angle = c.cosangle;
    float sinus_angle = c.sinangle;
    float fov = c.inv_tan;

    
    if (tb.z <= 0 && ta.z > 0) {
        vector stockage = ta;
        ta = tb;
        tb = stockage;
    }

    if (ta.z <= 0 && tb.z > 0) {
        vector na = ta;
        vector nb = tb;

        float az = ta.z;
        float bz = tb.z;
        float ax = ta.x;
        float ay = ta.y;
        float bx = tb.x;
        float by = tb.y;

        float mini = fmin(fmin(abs(ax), abs(ay)), fmin(abs(bx), abs(by)));
        mini = fov * mini;

        float t = (mini - az) / (bz - az);
        ta = add(scale(ta, t), scale(tb, 1-t));
    }
        
        

    if (ta.z > 0 && tb.z > 0) {
        ta = scale(ta, 1/(ta.z));
        tb = scale(tb, 1/(tb.z));

        ta = nvec(ta.x * cosinus_angle - ta.y * sinus_angle, ta.x * sinus_angle + ta.y * cosinus_angle, 0);
        tb = nvec(tb.x * cosinus_angle - tb.y * sinus_angle, tb.x * sinus_angle + tb.y * cosinus_angle, 0);



        float horschampa = 1/(inorm(ta));
        float horschampb = 1/(inorm(tb));
        int dontdraw = 1;

        if (horschampa > 1.5 && horschampb > 1.5) {
            float interpolation = ( -tb.x * (ta.x + tb.x) - tb.y * (ta.y + tb.y) ) / ( (ta.x - tb.x) * (ta.x - tb.x) +  (ta.y - tb.y) * (ta.y - tb.y) );
            if (interpolation <= 1 && interpolation >= 0) {
                vector orthogonal = add(scale(ta, interpolation), scale(tb, 1 - interpolation)); // On a une ligne perpendiculaire au segment AB.
                ta = scale(normalisation(sub(ta, tb)), 1.5);
                tb = scale(ta, -1);
                ta = add(ta, orthogonal);
                tb = add(tb, orthogonal);
                dontdraw = 0;
            } 
        } else if (horschampa > 1.5) {
            vector amb = sub(ta, tb); // a moins b
            amb = scale(normalisation(amb), 1.5);
            ta = add(amb, tb);
            dontdraw = 0;
        } else if (horschampb > 1.5) {
            vector bma = sub(tb, ta);
            bma = scale(normalisation(bma), 1.5);
            tb = add(bma, ta);
            dontdraw = 0;
        } else if (horschampa <= 1.5, horschampb <= 1.5) {
            dontdraw = 0;
        }

        if (dontdraw == 0) {
            float scalaire = 1024*fov;
            ta = scale(ta, scalaire);
            tb = scale(tb, scalaire);


            vector glider = sub(ta, tb);

            vector translation = nvec(512.5, 512.5, 0);

            ta = add(ta, translation);
            tb = add(tb, translation);

            float nrm = inorm(glider);
            float ite = pent(1/nrm) + 1; if (ite > 10000) {printf("erreur\n");};
            glider = normalisation(glider);
            int X;
            int Y;
        
            for (int i = 0; i < ite; i++) {
                X = pent(tb.x);
                Y = pent(tb.y);
                if (X > 0 && X < 1024 && Y > 0 && Y < 1024) {
                    SDL_RenderDrawPoint(d, X, 1024 - Y);
                }
                tb = add(tb, glider);
            }
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

void drawsphere(vector *todraw, cam camera, SDL_Renderer *d) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 48; j++) {
            vd(todraw[48*i + (j%48)], todraw[48*i + ( (j+1)%48 )], camera, d);
        }
    }
}

/*
void protoraster(triangle tri, cam camera, SDL_Renderer *d) {
    vector a = sub(tri.a, tri.c); // Notre axe X
    vector b = sub(tri.b, tri.c); // notre axe Y
    vector c = tri.c; // Le vecteur Y -> X

    vector ab = sub(a, b);
    

    float norme ab = inorm(sub(a, b));
    ab = scale(ab, norme);
    norme = pent(1/norme) + 1;

    int ratio;

    for (int i = 0; i < norme; i++) {
        ratio = i / (norme - 1);
        vd(add(a, c), )
    }  
}
*/


void drawcube(vector *todraw, cam camera, SDL_Renderer *d) {
    vd(todraw[0], todraw[1], camera, d);
    vd(todraw[0], todraw[2], camera, d);
    vd(todraw[2], todraw[3], camera, d);
    vd(todraw[1], todraw[3], camera, d);
    vd(todraw[4], todraw[0], camera, d);
    vd(todraw[5], todraw[1], camera, d);
    vd(todraw[6], todraw[2], camera, d);
    vd(todraw[7], todraw[3], camera, d);
    vd(todraw[4], todraw[5], camera, d);
    vd(todraw[4], todraw[6], camera, d);
    vd(todraw[5], todraw[7], camera, d);
    vd(todraw[6], todraw[7], camera, d);
}

int main(void) {
    cam camera;
    camera.pos = nvec(70, 10, 70);
    setcamdirection(&camera, nvec(-7, -1, -7));
    setcamangle(&camera, 0);
    setcamfov(&camera, 90);

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;


    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(1024, 1024, 0, &window, &renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);    
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);


    /* 
    for (int i = 0; i < 64; i++) {
        vector *kube = cube(10, 10*(i&3), 0, 10*((i&(0b1100))>>2));
        drawcube(kube, camera, renderer);
        free(kube);
    }
    */
    
    
    vector *kube = cube(10, 20, 10, 20);
    drawcube(kube, camera, renderer);

    SDL_RenderPresent(renderer);
    SDL_Delay(5000);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);    
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    /*
    vector *spheru = sphere(20, 0, 0, 0);
    drawsphere(spheru, camera, renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(5000);
    */


    vector travelling;

    for (int i = 0; i < 1500; i++) {
        travelling = nvec(2*cos((i*3.141592)/120), 1, 2*sin((i*3.141592)/120));
        setcamdirection(&camera, scale(travelling, -1));
        travelling = scale(travelling, 7);
        travelling = add(travelling, nvec(15, 0, 15));
        camera.pos = travelling;
        for (int j = 0; j < 64; j++) {
            vector *kube = cube(10, 10*(j&3), 0, 10*((j&(0b1100))>>2));
            drawcube(kube, camera, renderer);
            free(kube);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(17);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);    
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);


    }

    
    SDL_Quit();
    return 0;
}
