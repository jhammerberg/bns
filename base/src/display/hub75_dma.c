#include "hub75_dma.h"
#include "xmc_ccu4.h"
#include "xmc_dma.h"
#include "xmc_gpio.h"
#include "xmc_scu.h"

#define DMA         XMC_DMA0
#define DMA_CHANNEL 0U // channels 0-1 have the 32-byte FIFO
#define PACE_MODULE CCU42
#define PACE_SLICE  CCU42_CC40 // its SR0 drives DMA request line 0

static void pacing_timer_start(uint32_t write_hz) {
    const uint32_t ccu_hz = XMC_SCU_CLOCK_GetCcuClockFrequency();
    XMC_CCU4_Init(PACE_MODULE, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(PACE_MODULE);
    const XMC_CCU4_SLICE_COMPARE_CONFIG_t config = {
        .timer_mode = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
        .monoshot = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_REPEAT,
        .prescaler_initval = XMC_CCU4_SLICE_PRESCALER_1,
    };
    XMC_CCU4_SLICE_CompareInit(PACE_SLICE, &config);
    XMC_CCU4_SLICE_SetTimerPeriodMatch(PACE_SLICE, (uint16_t)(ccu_hz / write_hz - 1U));
    XMC_CCU4_EnableShadowTransfer(PACE_MODULE, XMC_CCU4_SHADOW_TRANSFER_SLICE_0);
    // period match -> service request 0 -> DMA request line 0 (no CPU interrupt)
    XMC_CCU4_SLICE_EnableEvent(PACE_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
    XMC_CCU4_SLICE_SetInterruptNode(PACE_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH,
                                    XMC_CCU4_SLICE_SR_ID_0);
    XMC_CCU4_EnableClock(PACE_MODULE, 0);
    XMC_CCU4_SLICE_StartTimer(PACE_SLICE);
}

void hub75_dma_init(uint32_t write_hz) {
    XMC_DMA_Init(DMA);
    const XMC_DMA_CH_CONFIG_t config = {
        .enable_interrupt = true,
        .dst_transfer_width = XMC_DMA_CH_TRANSFER_WIDTH_32,
        .src_transfer_width = XMC_DMA_CH_TRANSFER_WIDTH_32,
        .dst_address_count_mode = XMC_DMA_CH_ADDRESS_COUNT_MODE_NO_CHANGE,
        .src_address_count_mode = XMC_DMA_CH_ADDRESS_COUNT_MODE_INCREMENT,
        .dst_burst_length = XMC_DMA_CH_BURST_LENGTH_1,
        .src_burst_length = XMC_DMA_CH_BURST_LENGTH_1,
        .transfer_flow = XMC_DMA_CH_TRANSFER_FLOW_M2P_DMA,
        .src_addr = 0,
        .dst_addr = (uint32_t)&XMC_GPIO_PORT5->OMR,
        .block_size = 1,
        .transfer_type = XMC_DMA_CH_TRANSFER_TYPE_SINGLE_BLOCK,
        .priority = XMC_DMA_CH_PRIORITY_7,
        .dst_handshaking = XMC_DMA_CH_DST_HANDSHAKING_HARDWARE,
        .dst_peripheral_request = DMA0_PERIPHERAL_REQUEST_CCU42_SR0_0,
    };
    XMC_DMA_CH_Init(DMA, DMA_CHANNEL, &config);
    XMC_DMA_CH_EnableEvent(DMA, DMA_CHANNEL, XMC_DMA_CH_EVENT_BLOCK_TRANSFER_COMPLETE);
    pacing_timer_start(write_hz);
}

void hub75_dma_start(const uint32_t *words, uint32_t count) {
    XMC_DMA_CH_SetSourceAddress(DMA, DMA_CHANNEL, (uint32_t)words);
    XMC_DMA_CH_SetDestinationAddress(DMA, DMA_CHANNEL, (uint32_t)&XMC_GPIO_PORT5->OMR);
    XMC_DMA_CH_SetBlockSize(DMA, DMA_CHANNEL, count);
    XMC_DMA_CH_Enable(DMA, DMA_CHANNEL);
}

void hub75_dma_acknowledge(void) {
    XMC_DMA_CH_ClearEventStatus(DMA, DMA_CHANNEL, XMC_DMA_CH_EVENT_BLOCK_TRANSFER_COMPLETE);
}
