# MeMoCO lab project - Part I (CPLEX) and Part II (tabu search).
#
# CPLEX location: the department lab installation if present, otherwise the
# software share mounted on the student server. Override on other machines:
#     make CPX_BASE=/home/user/ibm/ILOG/CPLEX_Studio2212
CPX_LAB     = /opt/ibm/ILOG/CPLEX_Studio2211
CPX_SHARE   = /mnt/server1/0/Software/CPLEX/ibm/ILOG/CPLEX_Studio2211
CPX_BASE   ?= $(if $(wildcard $(CPX_LAB)),$(CPX_LAB),$(CPX_SHARE))
CPX_INCDIR  = $(CPX_BASE)/cplex/include
CPX_LIBDIR  = $(CPX_BASE)/cplex/lib/x86-64_linux/static_pic
CPX_LDFLAGS = -lcplex -lm -pthread -ldl

CC       = g++
CPPFLAGS = -g -Wall -O2 -std=c++11 -Icommon

all: tsp_cplex tsp_tabu

tsp_cplex: assignment_1/main.cpp assignment_1/tsp_model.cpp assignment_1/tsp_model.h assignment_1/cpxmacro.h common/instance.h common/timer.h
	$(CC) $(CPPFLAGS) -Iassignment_1 -I$(CPX_INCDIR) assignment_1/main.cpp assignment_1/tsp_model.cpp -o tsp_cplex -L$(CPX_LIBDIR) $(CPX_LDFLAGS)

tsp_tabu: assignment_2/main.cpp assignment_2/tabu_search.cpp assignment_2/tabu_search.h common/instance.h common/timer.h
	$(CC) $(CPPFLAGS) -Iassignment_2 assignment_2/main.cpp assignment_2/tabu_search.cpp -o tsp_tabu

clean:
	rm -f tsp_cplex tsp_tabu *.o

.PHONY: all clean
