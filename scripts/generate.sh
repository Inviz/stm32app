
cp ./src/configs/OD.h ./src/definitions/Base.h
node scripts/overlay.js ./src/definitions/Base_F1.c ./src/configs/OD.c ./src/configs/Base_F1.c 
node scripts/overlay.js ./src/definitions/Base_F4.c ./src/configs/OD.c ./src/configs/Base_F4.c 

node --experimental-modules scripts/types.js ./src/configs/Base.xdd ./src/configs/OD.h

