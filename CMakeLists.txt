cmake_minimum_required (VERSION 3.0.0)
project(driver_demo)

# path of kernel
set(KDIR /home/linux/linux-5.12)

# function of compile
function(compile_module obj)
  set(TARGET_NAME ${obj})
  # add target cp c h source code
  add_custom_target(${TARGET_NAME} ALL cp -f ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_BINARY_DIR}/
			COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_BINARY_DIR}/
  			COMMAND echo "compiling module ${obj}.ko..."
  		    )

  # Makefile中的 $(MODULE_NAME)-objs += demo_main.o
  set(depend_objlist "demo_main.o")
  
  # 设置编译命令
  add_custom_command(TARGET ${TARGET_NAME}
		     POST_BUILD
		     COMMAND echo "obj-m := ${obj}.o" > ${CMAKE_CURRENT_BINARY_DIR}/Makefile
		     COMMAND echo "${obj}-objs:=${depend_objlist}" >>${CMAKE_CURRENT_BINARY_DIR}/Makefile
		     COMMAND make -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} modules
		     )
endfunction()


compile_module(demo)
