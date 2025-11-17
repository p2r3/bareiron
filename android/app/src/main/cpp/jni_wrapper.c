#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <stdio.h>

#define LOG_TAG "BareIronJNI"

// Globale variabelen voor communicatie met de JVM
static JavaVM* g_vm = NULL;
static jobject g_service_obj = NULL; // Verwijst nu naar het ServerService object
static jmethodID g_mid_update_console = NULL;

// Declareer de main functie van je C-server
int main(int argc, char **argv);

// Variabelen voor het omleiden van stdout/stderr
static int pfd[2];
static pthread_t thr;
static int logging_started = 0;

static void *thread_func(void *arg) {
    // Koppel de huidige thread aan de Java Virtual Machine
    JNIEnv *env;
    if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to attach current thread");
        return NULL;
    }

    ssize_t rdsz;
    char buf[256];
    // Blijf lezen van de pipe waar stdout/stderr naartoe schrijft
    while ((rdsz = read(pfd[0], buf, sizeof(buf) - 1)) > 0) {
        if (rdsz > 0) {
            buf[rdsz] = 0; // Null-terminate de string
            jstring text = (*env)->NewStringUTF(env, buf);

            // Roep de 'updateConsole' methode aan op het opgeslagen ServerService object
            if (g_service_obj != NULL && g_mid_update_console != NULL) {
                (*env)->CallVoidMethod(env, g_service_obj, g_mid_update_console, text);
            }

            (*env)->DeleteLocalRef(env, text);
        }
    }

    // Koppel de thread los van de JVM
    (*g_vm)->DetachCurrentThread(g_vm);
    return 0;
}

int start_logging(JNIEnv *env, jobject service_instance) {
    if (logging_started) return 0;

    // Bewaar globale referenties die veilig zijn over threads heen
    (*env)->GetJavaVM(env, &g_vm);
    g_service_obj = (*env)->NewGlobalRef(env, service_instance);

    // Vind de 'updateConsole' methode op de Service klasse
    jclass service_class = (*env)->GetObjectClass(env, service_instance);
    g_mid_update_console = (*env)->GetMethodID(env, service_class, "updateConsole", "(Ljava/lang/String;)V");

    if (g_mid_update_console == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to find method 'updateConsole' on service class");
        return -1;
    }

    // Zet de pipe op om stdout en stderr om te leiden
    pipe(pfd);
    dup2(pfd[1], STDOUT_FILENO);
    dup2(pfd[1], STDERR_FILENO);
    setvbuf(stdout, NULL, _IONBF, 0); // Schakel buffering uit
    setvbuf(stderr, NULL, _IONBF, 0); // Schakel buffering uit

    // Start de logging thread die de pipe leest
    pthread_create(&thr, NULL, thread_func, NULL);
    logging_started = 1;

    return 0;
}

/*
 * De JNI-functie die wordt aangeroepen vanuit de ServerService.
 * De naam moet exact overeenkomen: Java_pakketnaam_Klassenaam_functienaam
 */
JNIEXPORT void JNICALL
Java_nl_willemdeprogrammeur_bareiron_ServerService_bareironMain(JNIEnv *env, jobject thiz) {
    // 'thiz' is hier een referentie naar de 'ServerService' instantie
    if (start_logging(env, thiz) == 0) {
        // Start de C-server. Deze functie blokkeert en keert nooit terug.
        main(0, NULL);
    }
}
