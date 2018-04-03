package dasher;
import org.junit.Test;
import org.junit.Before;
import static org.junit.Assert.*;

public class CAlphabetMapTest {

	CAlphabetMap map = new CAlphabetMap(5);

	@Before
	public void setUp(){
		map.Add("key", 2);
	}

	@Test
	public void gettingValueMapContains() {
		assertEquals(map.Get("key").symbol, 2);
	}

	@Test
	public void gettingValueMapDoesNotContain() {
		assertEquals(map.Get("notExists").symbol, 0);
	}
}
