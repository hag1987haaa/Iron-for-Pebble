import sys

with open('src/c/main.c', 'r') as f:
    text = f.read()

# 1. Move the declarations
decls = '''static const char *s_font_huge_time_key;
static const char *s_font_long_time_key;
static const char *s_font_colon_key;
static const char *s_font_mid_data_key;'''

text = text.replace(decls, '') # Remove from original place
text = text.replace('static GColor s_current_main_bg, s_current_main_fg;', decls + '\nstatic GColor s_current_main_bg, s_current_main_fg;')

# 2. Fix the defined(PBL_ROUND)
bad_code = 's_is_round_screen = (wt == 180 && h == 180) || (wt == 260 && h == 260) || defined(PBL_ROUND);'
good_code = '''#if defined(PBL_ROUND)
    s_is_round_screen = true;
#else
    s_is_round_screen = (wt == 180 && h == 180) || (wt == 260 && h == 260); 
#endif'''
text = text.replace(bad_code, good_code)

with open('src/c/main.c', 'w') as f:
    f.write(text)

print("Fixed")
