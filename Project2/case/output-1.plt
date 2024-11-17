set xrange [0:110]
set yrange [0:100]

set arrow from 17,25 to 17,62 nohead
set arrow from 17,62 to 49,62 nohead
set arrow from 49,62 to 49,82 nohead
set arrow from 49,82 to 56,82 nohead
set arrow from 56,82 to 72,82 nohead
set arrow from 72,82 to 72,27 nohead
set arrow from 72,27 to 97,27 nohead
set arrow from 97,27 to 97,38 nohead

# 顯示圖形
plot NaN

plot '-' with points pt 7 ps 1.5 notitle, '-' with points pt 5 ps 1.5 notitle
17 25
49 62
72 27
56 82
e
97 38
e