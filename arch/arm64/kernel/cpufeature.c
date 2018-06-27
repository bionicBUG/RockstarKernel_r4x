/*
 * Contains CPU feature definitions
 *
 * Copyright (C) 2015 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) "alternatives: " fmt

#include <linux/types.h>
#include <asm/cpu.h>
#include <asm/cpufeature.h>
#include <asm/processor.h>

static bool
feature_matches(u64 reg, const struct arm64_cpu_capabilities *entry)
{
	int val = cpuid_feature_extract_field(reg, entry->field_pos);

	return val >= entry->min_field_value;
}

static bool
has_id_aa64pfr0_feature(const struct arm64_cpu_capabilities *entry)
{
	u64 val;

	val = read_cpuid(id_aa64pfr0_el1);
	return feature_matches(val, entry);
}

static bool __maybe_unused
has_id_aa64mmfr1_feature(const struct arm64_cpu_capabilities *entry)
{
	u64 val;

	val = read_cpuid(id_aa64mmfr1_el1);
	return feature_matches(val, entry);
}

static const struct arm64_cpu_capabilities arm64_features[] = {
	{
		.desc = "GIC system register CPU interface",
		.capability = ARM64_HAS_SYSREG_GIC_CPUIF,
		.matches = has_id_aa64pfr0_feature,
		.field_pos = 24,
		.min_field_value = 1,
	},
#ifdef CONFIG_ARM64_PAN
	{
		.desc = "Privileged Access Never",
		.capability = ARM64_HAS_PAN,
		.matches = has_id_aa64mmfr1_feature,
		.field_pos = 20,
		.min_field_value = 1,
		.enable = cpu_enable_pan,
	},
#endif /* CONFIG_ARM64_PAN */
	{},
};

void check_cpu_capabilities(const struct arm64_cpu_capabilities *caps,
			    const char *info)
{
	int i;

	for (i = 0; caps[i].desc; i++) {
		if (!caps[i].matches(&caps[i]))
			continue;

		if (!cpus_have_cap(caps[i].capability))
			pr_info("%s %s\n", info, caps[i].desc);
		cpus_set_cap(caps[i].capability);
	}


/*
 * Run through the enabled capabilities and enable() it on all active
 * CPUs
 */
void __init enable_cpu_capabilities(const struct arm64_cpu_capabilities *caps)
{
	int i;

	for (i = 0; caps[i].matches; i++)
		if (caps[i].enable && cpus_have_cap(caps[i].capability))
			on_each_cpu(caps[i].enable, (void *)&(caps[i]), true);
}

#ifdef CONFIG_HOTPLUG_CPU

/*
 * Flag to indicate if we have computed the system wide
 * capabilities based on the boot time active CPUs. This
 * will be used to determine if a new booting CPU should
 * go through the verification process to make sure that it
 * supports the system capabilities, without using a hotplug
 * notifier.
 */
static bool sys_caps_initialised;

static inline void set_sys_caps_initialised(void)
{
	sys_caps_initialised = true;
}

/*
 * __raw_read_system_reg() - Used by a STARTING cpu before cpuinfo is populated.
 */
static u64 __raw_read_system_reg(u32 sys_id)
{
	switch (sys_id) {
	case SYS_ID_PFR0_EL1:		return read_cpuid(SYS_ID_PFR0_EL1);
	case SYS_ID_PFR1_EL1:		return read_cpuid(SYS_ID_PFR1_EL1);
	case SYS_ID_DFR0_EL1:		return read_cpuid(SYS_ID_DFR0_EL1);
	case SYS_ID_MMFR0_EL1:		return read_cpuid(SYS_ID_MMFR0_EL1);
	case SYS_ID_MMFR1_EL1:		return read_cpuid(SYS_ID_MMFR1_EL1);
	case SYS_ID_MMFR2_EL1:		return read_cpuid(SYS_ID_MMFR2_EL1);
	case SYS_ID_MMFR3_EL1:		return read_cpuid(SYS_ID_MMFR3_EL1);
	case SYS_ID_ISAR0_EL1:		return read_cpuid(SYS_ID_ISAR0_EL1);
	case SYS_ID_ISAR1_EL1:		return read_cpuid(SYS_ID_ISAR1_EL1);
	case SYS_ID_ISAR2_EL1:		return read_cpuid(SYS_ID_ISAR2_EL1);
	case SYS_ID_ISAR3_EL1:		return read_cpuid(SYS_ID_ISAR3_EL1);
	case SYS_ID_ISAR4_EL1:		return read_cpuid(SYS_ID_ISAR4_EL1);
	case SYS_ID_ISAR5_EL1:		return read_cpuid(SYS_ID_ISAR4_EL1);
	case SYS_MVFR0_EL1:		return read_cpuid(SYS_MVFR0_EL1);
	case SYS_MVFR1_EL1:		return read_cpuid(SYS_MVFR1_EL1);
	case SYS_MVFR2_EL1:		return read_cpuid(SYS_MVFR2_EL1);

	case SYS_ID_AA64PFR0_EL1:	return read_cpuid(SYS_ID_AA64PFR0_EL1);
	case SYS_ID_AA64PFR1_EL1:	return read_cpuid(SYS_ID_AA64PFR0_EL1);
	case SYS_ID_AA64DFR0_EL1:	return read_cpuid(SYS_ID_AA64DFR0_EL1);
	case SYS_ID_AA64DFR1_EL1:	return read_cpuid(SYS_ID_AA64DFR0_EL1);
	case SYS_ID_AA64MMFR0_EL1:	return read_cpuid(SYS_ID_AA64MMFR0_EL1);
	case SYS_ID_AA64MMFR1_EL1:	return read_cpuid(SYS_ID_AA64MMFR1_EL1);
	case SYS_ID_AA64MMFR2_EL1:	return read_cpuid(SYS_ID_AA64MMFR2_EL1);
	case SYS_ID_AA64ISAR0_EL1:	return read_cpuid(SYS_ID_AA64ISAR0_EL1);
	case SYS_ID_AA64ISAR1_EL1:	return read_cpuid(SYS_ID_AA64ISAR1_EL1);

	case SYS_CNTFRQ_EL0:		return read_cpuid(SYS_CNTFRQ_EL0);
	case SYS_CTR_EL0:		return read_cpuid(SYS_CTR_EL0);
	case SYS_DCZID_EL0:		return read_cpuid(SYS_DCZID_EL0);
	default:
		BUG();
		return 0;
	}
}

/*
 * Park the CPU which doesn't have the capability as advertised
 * by the system.
 */
static void fail_incapable_cpu(char *cap_type,
				 const struct arm64_cpu_capabilities *cap)
{
	int cpu = raw_smp_processor_id();

	pr_crit("CPU%d: missing %s : %s\n", cpu, cap_type, cap->desc);
	/* Mark this CPU absent */
	set_cpu_present(cpu, 0);

	/* Check if we can park ourselves */
	if (cpu_ops[cpu] && cpu_ops[cpu]->cpu_die)
		cpu_ops[cpu]->cpu_die(cpu);
	asm(
	"1:	wfe\n"
	"	wfi\n"
	"	b	1b");
}

/*
 * Run through the enabled system capabilities and enable() it on this CPU.
 * The capabilities were decided based on the available CPUs at the boot time.
 * Any new CPU should match the system wide status of the capability. If the
 * new CPU doesn't have a capability which the system now has enabled, we
 * cannot do anything to fix it up and could cause unexpected failures. So
 * we park the CPU.
 */
void verify_local_cpu_capabilities(void)
{
	int i;
	const struct arm64_cpu_capabilities *caps;

	/*
	 * If we haven't computed the system capabilities, there is nothing
	 * to verify.
	 */
	if (!sys_caps_initialised)
		return;

	caps = arm64_features;
	for (i = 0; caps[i].matches; i++) {
		if (!cpus_have_cap(caps[i].capability) || !caps[i].sys_reg)
			continue;
		/*
		 * If the new CPU misses an advertised feature, we cannot proceed
		 * further, park the cpu.
		 */
		if (!feature_matches(__raw_read_system_reg(caps[i].sys_reg), &caps[i]))
			fail_incapable_cpu("arm64_features", &caps[i]);
		if (caps[i].enable)
			caps[i].enable((void *)&(caps[i]));
	}

	/* second pass allows enable() to consider interacting capabilities */
	for (i = 0; caps[i].desc; i++) {
		if (cpus_have_cap(caps[i].capability) && caps[i].enable)
			caps[i].enable();
	}
}

void check_local_cpu_features(void)
{
	check_cpu_capabilities(arm64_features, "detected feature");
}
