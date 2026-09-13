/* A Zephyr shell that survives being a remote board.
 *
 * The shell subsystem runs on its own, so main only has to handle the one
 * thing a remote board cannot do without: getting back into a state where it
 * can be reflashed. See the board conf files for which boards need that.
 */

#include <zephyr/kernel.h>

int main(void)
{
	return 0;
}
