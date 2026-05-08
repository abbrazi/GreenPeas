install(TARGETS gp RUNTIME COMPONENT gprt)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
