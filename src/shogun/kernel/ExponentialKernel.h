/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Soeren Sonnenburg, Sergey Lisitsyn, Evan Shelhamer, Yuyu Zhang
 */

#ifndef _EXPONENTIALKERNEL_H___
#define _EXPONENTIALKERNEL_H___

#include <shogun/lib/config.h>

#include <shogun/lib/common.h>
#include <shogun/kernel/DotKernel.h>
#include <shogun/features/DotFeatures.h>
#include <shogun/distance/Distance.h>

namespace shogun
{
	class CDotFeatures;
/** @brief The Exponential Kernel, closely related to the Gaussian Kernel
 * computed on CDotFeatures.
 *
 * It is computed as
 *
 * \f[
 * k({\bf x},{\bf x'})= exp(-\frac{||{\bf x}-{\bf x'}||}{\tau})
 * \f]
 *
 * where \f$\tau\f$ is the kernel width.
 */
class CExponentialKernel: public CDotKernel
{
	public:
		/** default constructor
		 *
		 */
		CExponentialKernel();

		/** constructor
		 *
		 * @param l features of left-hand side
		 * @param r features of right-hand side
		 * @param width width
		 * @param distance distance to be used
		 * @param size cache size
		 */
		CExponentialKernel(CDotFeatures* l, CDotFeatures* r,
			float64_t width, CDistance* distance, int32_t size);

		/** destructor */
		virtual ~CExponentialKernel();

		/** initialize kernel
		 *
		 * @param l features of left-hand side
		 * @param r features of right-hand side
		 * @return if initializing was successful
		 */
		virtual bool init(CFeatures* l, CFeatures* r);

		/** clean up kernel */
		virtual void cleanup();

		/** return what type of kernel we are
		 *
		 * @return kernel type EXPONENTIAL
		 */
		virtual EKernelType get_kernel_type() { return K_EXPONENTIAL; }

		/** return the kernel's name
		 *
		 * @return name Exponential
		 */
		virtual const char* get_name() const { return "ExponentialKernel"; }

		/** return the kernel's width
		 *
		 * @return kernel width
		 */
		virtual float64_t get_width() const
		{
			return m_width;
		}

	protected:
		/** compute kernel function for features a and b
		 * idx_{a,b} denote the index of the feature vectors
		 * in the corresponding feature object
		 *
		 * @param idx_a index a
		 * @param idx_b index b
		 * @return computed kernel function at indices a,b
		 */
		virtual float64_t compute(int32_t idx_a, int32_t idx_b);

		/** Can (optionally) be overridden to post-initialize some
		 *  member variables which are not PARAMETER::ADD'ed.  Make
		 *  sure that at first the overridden method
		 *  BASE_CLASS::LOAD_SERIALIZABLE_POST is called.
		 *
		 *  @exception ShogunException Will be thrown if an error
		 *                             occurres.
		 */
		virtual void load_serializable_post() throw (ShogunException);

	private:
		void init();

	protected:
		/** distance **/
		CDistance* m_distance;
		/** width */
		float64_t m_width;
};
}
#endif /* _EXPONENTIALKERNEL_H__ */
