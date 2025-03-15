#pragma once

void isr_de(void);
void isr_db(void);
void isr_nmi(void);
void isr_bp(void);
void isr_of(void);
void isr_br(void);
void isr_ud(void);
void isr_nm(void);
void isr_df(void);
void isr_cso(void);
void isr_ts(void);
void isr_np(void);
void isr_ss(void);
void isr_gp(void);
void isr_pf(void);
void isr_mf(void);
void isr_ac(void);
void isr_mc(void);
void isr_xm(void);
void isr_ve(void);
void isr_cp(void);
void isr_reserved(void);
void isr_timer(void);

void isr_default(void);