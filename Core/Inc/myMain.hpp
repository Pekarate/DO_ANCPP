/*
 * myMain.h
 *
 *  Created on: Apr 16, 2023
 *      Author: ADMIN
 */

#ifndef INC_MYMAIN_HPP_
#define INC_MYMAIN_HPP_

void init();
#ifdef __cplusplus
extern "C"
{
#endif
    void initC();  // C function to call into Cpp event loop from main
    void loopC();
#ifdef __cplusplus
}
#endif
#endif /* INC_MYMAIN_HPP_ */
