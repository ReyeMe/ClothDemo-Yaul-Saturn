#ifndef __CLOTH_SIM_H__
#define __CLOTH_SIM_H__

/** @brief Initialize cloth simulation
 *  @return Number of quads in cloth
 */
uint32_t ClothSimInitialize();

/** @brief Update simulation
 */
void ClothSimTick();

/** @brief Move cloth
 *  @param digital Controller input
 */
void ClothMove(smpc_peripheral_digital_t *digital);

#endif
