# How To Use QA Test Scripts #

## Run all UnitTests and automated Test Cases for all bridge projects
1. Start Daz Studio.
2. Open the Script IDE Pane.
3. Load "Test/RunAllTests.dsa".
4. Configure "sCommonPath" on line 10 to point to the correct absolute path containing the RunAllTests.dsa script.
5. Configure "Global_sOutputPath" on line 13 to point to the absolute path of the "Test/Results/" folder.
6. Make sure the correct absolute path is specified for DzUnreal and DzUnity scripts.  Comment out any sections you do not wish to run.
7. Run the script by clicking "Execute" or pressing <F5>.

## Run only UnitTests
1. Start Daz Studio.
2. Open the Script IDE Pane.
3. Load "Test/UnitTests/RunUnitTests.dsa".
4. Configure the "sIncludePath" to point to the correct absolute path containing the RunUnitTests.dsa script.
5. Configure the "sOutputPath" to pont to the absolute path of the "Test/Results" folder.
6. Run the script.

## Run all Automated Test Cases for a specific bridge project
1. Start Daz Studio.
2. Open the Script IDE Pane.
3. Load "<DzBridge-project>/Test/TestCases/test_runner.dsa" for whatever desired bridge project, such as DazToUnreal or DazToUnity.
4. Configure "sIncludePath" to point to the correct absolute path containing the test_runner.dsa script.
5. Configure "Global_sOutputPath" to point to the absolute path of the "<DzBridge-project>/Test/Results/" folder.
6. Run the script.

## Run a single Automated Test Case for a specific bridge project
1. Start Daz Studio.
2. Open the Script IDE Pane.
3. Load "<DzBridge-project>/Test/TestCases/test_runner--single.dsa" for whatever desired bridge project, such as DazToUnreal or DazToUnity.
4. Configure "sIncludePath" to point to the correct absolute path containing the test_runner--single.dsa script.
5. Configure "Global_sOutputPath" to point to the absolute path of the "<DzBridge-project>/Test/Results/" folder.
6. Copy-Paste or uncomment the line for the desired Test Case(s) you wish to run.
