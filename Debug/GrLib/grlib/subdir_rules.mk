################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
GrLib/grlib/%.obj: ../GrLib/grlib/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccs1220/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted -O3 --use_hw_mpy=F5 --include_path="/Applications/ti/ccs1220/ccs/ccs_base/msp430/include" --include_path="/Users/reedchen/Documents/workspace/final_project" --include_path="/Users/reedchen/Documents/workspace/final_project/driverlib/MSP430F5xx_6xx" --include_path="/Users/reedchen/Documents/workspace/final_project/GrLib/grlib" --include_path="/Users/reedchen/Documents/workspace/final_project/GrLib/fonts" --include_path="/Applications/ti/ccs1220/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power="none" --define=__MSP430F5529__ -g --gcc --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --large_memory_model --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="GrLib/grlib/$(basename $(<F)).d_raw" --obj_directory="GrLib/grlib" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


