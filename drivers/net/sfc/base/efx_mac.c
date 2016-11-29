/*
 * Copyright (c) 2007-2016 Solarflare Communications Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the FreeBSD Project.
 */

#include "efx.h"
#include "efx_impl.h"

	__checkReturn			efx_rc_t
efx_mac_pdu_set(
	__in				efx_nic_t *enp,
	__in				size_t pdu)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	uint32_t old_pdu;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);
	EFSYS_ASSERT(emop != NULL);

	if (pdu < EFX_MAC_PDU_MIN) {
		rc = EINVAL;
		goto fail1;
	}

	if (pdu > EFX_MAC_PDU_MAX) {
		rc = EINVAL;
		goto fail2;
	}

	old_pdu = epp->ep_mac_pdu;
	epp->ep_mac_pdu = (uint32_t)pdu;
	if ((rc = emop->emo_pdu_set(enp)) != 0)
		goto fail3;

	return (0);

fail3:
	EFSYS_PROBE(fail3);

	epp->ep_mac_pdu = old_pdu;

fail2:
	EFSYS_PROBE(fail2);
fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

	__checkReturn	efx_rc_t
efx_mac_pdu_get(
	__in		efx_nic_t *enp,
	__out		size_t *pdu)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	efx_rc_t rc;

	if ((rc = emop->emo_pdu_get(enp, pdu)) != 0)
		goto fail1;

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

	__checkReturn			efx_rc_t
efx_mac_addr_set(
	__in				efx_nic_t *enp,
	__in				uint8_t *addr)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	uint8_t old_addr[6];
	uint32_t oui;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if (EFX_MAC_ADDR_IS_MULTICAST(addr)) {
		rc = EINVAL;
		goto fail1;
	}

	oui = addr[0] << 16 | addr[1] << 8 | addr[2];
	if (oui == 0x000000) {
		rc = EINVAL;
		goto fail2;
	}

	EFX_MAC_ADDR_COPY(old_addr, epp->ep_mac_addr);
	EFX_MAC_ADDR_COPY(epp->ep_mac_addr, addr);
	if ((rc = emop->emo_addr_set(enp)) != 0)
		goto fail3;

	return (0);

fail3:
	EFSYS_PROBE(fail3);

	EFX_MAC_ADDR_COPY(epp->ep_mac_addr, old_addr);

fail2:
	EFSYS_PROBE(fail2);
fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

	__checkReturn			efx_rc_t
efx_mac_filter_set(
	__in				efx_nic_t *enp,
	__in				boolean_t all_unicst,
	__in				boolean_t mulcst,
	__in				boolean_t all_mulcst,
	__in				boolean_t brdcst)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	boolean_t old_all_unicst;
	boolean_t old_mulcst;
	boolean_t old_all_mulcst;
	boolean_t old_brdcst;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	old_all_unicst = epp->ep_all_unicst;
	old_mulcst = epp->ep_mulcst;
	old_all_mulcst = epp->ep_all_mulcst;
	old_brdcst = epp->ep_brdcst;

	epp->ep_all_unicst = all_unicst;
	epp->ep_mulcst = mulcst;
	epp->ep_all_mulcst = all_mulcst;
	epp->ep_brdcst = brdcst;

	if ((rc = emop->emo_reconfigure(enp)) != 0)
		goto fail1;

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	epp->ep_all_unicst = old_all_unicst;
	epp->ep_mulcst = old_mulcst;
	epp->ep_all_mulcst = old_all_mulcst;
	epp->ep_brdcst = old_brdcst;

	return (rc);
}

	__checkReturn			efx_rc_t
efx_mac_drain(
	__in				efx_nic_t *enp,
	__in				boolean_t enabled)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);
	EFSYS_ASSERT(emop != NULL);

	if (epp->ep_mac_drain == enabled)
		return (0);

	epp->ep_mac_drain = enabled;

	if ((rc = emop->emo_reconfigure(enp)) != 0)
		goto fail1;

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

	__checkReturn	efx_rc_t
efx_mac_up(
	__in		efx_nic_t *enp,
	__out		boolean_t *mac_upp)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if ((rc = emop->emo_up(enp, mac_upp)) != 0)
		goto fail1;

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

	__checkReturn			efx_rc_t
efx_mac_fcntl_set(
	__in				efx_nic_t *enp,
	__in				unsigned int fcntl,
	__in				boolean_t autoneg)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	const efx_phy_ops_t *epop = epp->ep_epop;
	unsigned int old_fcntl;
	boolean_t old_autoneg;
	unsigned int old_adv_cap;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if ((fcntl & ~(EFX_FCNTL_RESPOND | EFX_FCNTL_GENERATE)) != 0) {
		rc = EINVAL;
		goto fail1;
	}

	/*
	 * Ignore a request to set flow control auto-negotiation
	 * if the PHY doesn't support it.
	 */
	if (~epp->ep_phy_cap_mask & (1 << EFX_PHY_CAP_AN))
		autoneg = B_FALSE;

	old_fcntl = epp->ep_fcntl;
	old_autoneg = epp->ep_fcntl_autoneg;
	old_adv_cap = epp->ep_adv_cap_mask;

	epp->ep_fcntl = fcntl;
	epp->ep_fcntl_autoneg = autoneg;

	/*
	 * Always encode the flow control settings in the advertised
	 * capabilities even if we are not trying to auto-negotiate
	 * them and reconfigure both the PHY and the MAC.
	 */
	if (fcntl & EFX_FCNTL_RESPOND)
		epp->ep_adv_cap_mask |=    (1 << EFX_PHY_CAP_PAUSE |
					    1 << EFX_PHY_CAP_ASYM);
	else
		epp->ep_adv_cap_mask &=   ~(1 << EFX_PHY_CAP_PAUSE |
					    1 << EFX_PHY_CAP_ASYM);

	if (fcntl & EFX_FCNTL_GENERATE)
		epp->ep_adv_cap_mask ^= (1 << EFX_PHY_CAP_ASYM);

	if ((rc = epop->epo_reconfigure(enp)) != 0)
		goto fail2;

	if ((rc = emop->emo_reconfigure(enp)) != 0)
		goto fail3;

	return (0);

fail3:
	EFSYS_PROBE(fail3);

fail2:
	EFSYS_PROBE(fail2);

	epp->ep_fcntl = old_fcntl;
	epp->ep_fcntl_autoneg = old_autoneg;
	epp->ep_adv_cap_mask = old_adv_cap;

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

			void
efx_mac_fcntl_get(
	__in		efx_nic_t *enp,
	__out		unsigned int *fcntl_wantedp,
	__out		unsigned int *fcntl_linkp)
{
	efx_port_t *epp = &(enp->en_port);
	unsigned int wanted = 0;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	/*
	 * Decode the requested flow control settings from the PHY
	 * advertised capabilities.
	 */
	if (epp->ep_adv_cap_mask & (1 << EFX_PHY_CAP_PAUSE))
		wanted = EFX_FCNTL_RESPOND | EFX_FCNTL_GENERATE;
	if (epp->ep_adv_cap_mask & (1 << EFX_PHY_CAP_ASYM))
		wanted ^= EFX_FCNTL_GENERATE;

	*fcntl_linkp = epp->ep_fcntl;
	*fcntl_wantedp = wanted;
}

	__checkReturn	efx_rc_t
efx_mac_multicast_list_set(
	__in				efx_nic_t *enp,
	__in_ecount(6*count)		uint8_t const *addrs,
	__in				int count)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	uint8_t	*old_mulcst_addr_list = NULL;
	uint32_t old_mulcst_addr_count;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if (count > EFX_MAC_MULTICAST_LIST_MAX) {
		rc = EINVAL;
		goto fail1;
	}

	old_mulcst_addr_count = epp->ep_mulcst_addr_count;
	if (old_mulcst_addr_count > 0) {
		/* Allocate memory to store old list (instead of using stack) */
		EFSYS_KMEM_ALLOC(enp->en_esip,
				old_mulcst_addr_count * EFX_MAC_ADDR_LEN,
				old_mulcst_addr_list);
		if (old_mulcst_addr_list == NULL) {
			rc = ENOMEM;
			goto fail2;
		}

		/* Save the old list in case we need to rollback */
		memcpy(old_mulcst_addr_list, epp->ep_mulcst_addr_list,
			old_mulcst_addr_count * EFX_MAC_ADDR_LEN);
	}

	/* Store the new list */
	memcpy(epp->ep_mulcst_addr_list, addrs,
		count * EFX_MAC_ADDR_LEN);
	epp->ep_mulcst_addr_count = count;

	if ((rc = emop->emo_multicast_list_set(enp)) != 0)
		goto fail3;

	if (old_mulcst_addr_count > 0) {
		EFSYS_KMEM_FREE(enp->en_esip,
				old_mulcst_addr_count * EFX_MAC_ADDR_LEN,
				old_mulcst_addr_list);
	}

	return (0);

fail3:
	EFSYS_PROBE(fail3);

	/* Restore original list on failure */
	epp->ep_mulcst_addr_count = old_mulcst_addr_count;
	if (old_mulcst_addr_count > 0) {
		memcpy(epp->ep_mulcst_addr_list, old_mulcst_addr_list,
			old_mulcst_addr_count * EFX_MAC_ADDR_LEN);

		EFSYS_KMEM_FREE(enp->en_esip,
				old_mulcst_addr_count * EFX_MAC_ADDR_LEN,
				old_mulcst_addr_list);
	}

fail2:
	EFSYS_PROBE(fail2);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);

}

	__checkReturn	efx_rc_t
efx_mac_filter_default_rxq_set(
	__in		efx_nic_t *enp,
	__in		efx_rxq_t *erp,
	__in		boolean_t using_rss)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;
	efx_rc_t rc;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if (emop->emo_filter_default_rxq_set != NULL) {
		rc = emop->emo_filter_default_rxq_set(enp, erp, using_rss);
		if (rc != 0)
			goto fail1;
	}

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}

			void
efx_mac_filter_default_rxq_clear(
	__in		efx_nic_t *enp)
{
	efx_port_t *epp = &(enp->en_port);
	const efx_mac_ops_t *emop = epp->ep_emop;

	EFSYS_ASSERT3U(enp->en_magic, ==, EFX_NIC_MAGIC);
	EFSYS_ASSERT3U(enp->en_mod_flags, &, EFX_MOD_PORT);

	if (emop->emo_filter_default_rxq_clear != NULL)
		emop->emo_filter_default_rxq_clear(enp);
}


	__checkReturn			efx_rc_t
efx_mac_select(
	__in				efx_nic_t *enp)
{
	efx_port_t *epp = &(enp->en_port);
	efx_mac_type_t type = EFX_MAC_INVALID;
	const efx_mac_ops_t *emop;
	int rc = EINVAL;

	switch (enp->en_family) {

	default:
		rc = EINVAL;
		goto fail1;
	}

	EFSYS_ASSERT(type != EFX_MAC_INVALID);
	EFSYS_ASSERT3U(type, <, EFX_MAC_NTYPES);
	EFSYS_ASSERT(emop != NULL);

	epp->ep_emop = emop;
	epp->ep_mac_type = type;

	return (0);

fail1:
	EFSYS_PROBE1(fail1, efx_rc_t, rc);

	return (rc);
}


