if(PROJECT_IS_TOP_LEVEL)
  option(GP_DEVELOPER_MODE "Enable developer mode" OFF)
endif()

set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
  option(
    GP_INCLUDES_WITH_SYSTEM 
    "Use SYSTEM modifier for GreenPeas' includes, disabling warnings" 
    ON
  )
  mark_as_advanced(GP_INCLUDES_WITH_SYSTEM)
  if(GP_INCLUDES_WITH_SYSTEM)
    set(warning_guard SYSTEM)
  endif()
endif()
