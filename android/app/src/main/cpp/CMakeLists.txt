# Minimale CMake-versie
cmake_minimum_required(VERSION 3.18.1)

# Projectnaam (moet overeenkomen met System.loadLibrary in MainActivity)
project("bareiron")

# --- Pad naar bronbestanden en headers ---
# Voeg de 'include' map toe zodat de compiler je .h bestanden kan vinden.
include_directories(include)

# Vind automatisch alle .c bestanden in de 'c' map.
file(GLOB SOURCE_FILES "c/*.c")
# ----------------------------------------

# Voeg de native library toe.
# Belangrijk: Geef hier ALLE bronbestanden op.
add_library(
        bareiron
        SHARED
        jni_wrapper.c      # De JNI wrapper
        ${SOURCE_FILES}    # Alle .c bestanden uit de 'c' map
)

# Vind en link de Android logging-bibliotheek.
# Dit is nodig voor de jni_wrapper.c om output naar Logcat te sturen.
find_library(
        log-lib
        log
)
target_link_libraries(
        bareiron
        ${log-lib}
)
