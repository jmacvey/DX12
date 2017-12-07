:: BATCH SCRIPT TO CONVERT ALL BOLT FRAMES TO DDS FORMAT
:: -y -- FORCE OVERWRITE
:: -o -- OUTPUT DIRECTORY
:: -c color - TREAT COLOR AS ALPHA

@echo off

SETLOCAL
SET outputDirectory="boltDDSFiles"

IF NOT EXIST %outputDirectory% (
	mkdir %outputDirectory%
)
 
for /l %%x in (1, 1, 60) do (
	if %%x leq 9 (
		texconv Bolt00%%x.bmp -y -o %outputDirectory% -c 000000
	) else (
		texconv Bolt0%%x.bmp -y -o %outputDirectory% -c 000000
	)
)