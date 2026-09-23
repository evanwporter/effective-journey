#ifndef CONFIG_H
#define CONFIG_H

/* number of clients in master area */
static unsigned int nmaster = 1;

/* master area size [0.05..0.95] */
static float mfact = 0.5;

/* Layout definitions */
static const Layout layouts[] = {
    /* symbol     arrange function */
    {"[]=", tile}, /* tiling layout */
    {"><>", NULL}, /* floating layout (no arrange function) */
};

#endif  // CONFIG_H
