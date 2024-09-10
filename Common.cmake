enable_language(CXX)
cmake_minimum_required(VERSION 3.5)

#宏定义，添加一个包
#name：是一个不带双引号的字符
#dir：是一个不带引号的路径
macro(add_package name dir)
	include_directories(${ARGV1})
	set(hfind ${ARGV1})
	string(APPEND hfind "/*.h")
	set(cppfind ${ARGV1})
	string(APPEND cppfind "/*.cpp")
	set(cfind ${ARGV1})
	string(APPEND cfind "/*.c")
	set(ccfind ${ARGV1})
	string(APPEND ccfind "/*.cc")
	
	file(GLOB name_header ${hfind})
	file(GLOB name_cpp ${cppfind})
	file(GLOB name_c ${cfind})
	file(GLOB name_cc ${ccfind})

	set(all_file ${all_file} ${name_header} ${name_cpp} ${name_c} ${name_cc})
	source_group(${ARGV0} FILES ${name_header} ${name_cpp} ${name_c} ${name_cc})
endmacro(add_package)

#宏定义，添加一个包
#name：是一个不带双引号的字符
#dir：是一个不带引号的路径
macro(add_package_all name dir)
	include_directories(${ARGV1})
	set(hfind ${ARGV1})
	string(APPEND hfind "/*.*")
	file(GLOB_RECURSE name_header ${hfind})
	source_group(${ARGV0} FILES ${name_header} )
	set(all_file ${all_file} ${name_header} )
endmacro(add_package_all)

#宏定义，添加一个包,
#ARGV0 name：是一个不带双引号的字符
#ARGV1 dir：是一个不带引号的路径
#ARGV2 match：文件匹配表达式
macro(add_match_package name dir match)
	include_directories(${ARGV1})
	set(mfind ${ARGV1})
	string(APPEND mfind ${ARGV2})
	#message("mfind = " ${mfind})
	file(GLOB match_files ${mfind})
	source_group(${ARGV0} FILES ${match_files})
	set(all_file ${all_file} ${match_files})
endmacro(add_match_package)

#宏定义，递归添加一个包
#name：是一个不带双引号的字符
#dir：是一个不带引号的路径
macro(add_package_recurse name dir)
	include_directories(${dir})
	file(GLOB children ${dir}/*)
	foreach(child ${children})
		if(IS_DIRECTORY ${child})
			include_directories(${child})
		endif()
	endforeach()
	set(hfind ${ARGV1})
	string(APPEND hfind "/*.h")
	set(hppfind ${ARGV1})
	string(APPEND hppfind "/*.hpp")
	set(cppfind ${ARGV1})
	string(APPEND cppfind "/*.cpp")
	set(ccfind ${ARGV1})
	string(APPEND cfind "/*.cc")
	file(GLOB_RECURSE name_header ${hfind})
	file(GLOB_RECURSE name_hpp ${hppfind})
	file(GLOB_RECURSE name_cpp ${cppfind})
	file(GLOB_RECURSE name_cc ${ccfind})
	source_group(${ARGV0} FILES  ${name_header} ${name_hpp} ${name_cpp} ${name_cc})
	set(all_file ${all_file} ${name_header}  ${name_hpp} ${name_cpp} ${name_cc})
endmacro(add_package_recurse)


#只添加源文件(cpp、cc)，不包含头文件也不包含路径
macro(add_sources name dir)
	set(cppfind ${ARGV1})
	string(APPEND cppfind "/*.cpp")
	set(ccfind ${ARGV1})
	string(APPEND cfind "/*.cc")
	
	file(GLOB name_cpp ${cppfind})
	file(GLOB name_cc ${ccfind})

	set(all_file ${all_file} ${name_cpp} ${name_cc})
	source_group(${ARGV0} FILES ${name_cpp} ${name_cc})
endmacro(add_sources)




