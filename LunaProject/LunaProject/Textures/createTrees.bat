:: Convert bmp to txt

@echo off

SETLOAD
SET outputDirectory="trees"

IF NOT EXIST %outputDirectory% (
	mkdir %outputDirectory%
)

for /l %%x in (0, 1, 2) do (
	texconv tree%%x.bmp -y -o %outputDirectory% -c 000000 -m 1 -f BC3_UNORM
)

SET cwd=%cd%

cd %outputDirectory%

texassemble array -o -y treeArray.dds tree0.DDS tree1.DDS tree2.DDS tree2.DDS

texconv treeArray.dds -y -c 000000 -m 10
:: texconv treeArray.dds -y -c 000000

cd %cwd%