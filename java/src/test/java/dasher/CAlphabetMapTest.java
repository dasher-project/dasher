package dasher;
import org.junit.Test;
import org.junit.Before;
import static org.junit.Assert.*;

public class CAlphabetMapTest {

	CAlphabetMap map = new CAlphabetMap(5);
	SSymbol symbol;
	SSymbol newSymbol;

	@Before
	public void init(){
		symbol = new SSymbol();
		newSymbol = new SSymbol();
		map.Add("key", 2);
		symbol.symbol = 2;
		symbol.prefix = false;
		newSymbol.symbol = 0;
		newSymbol.prefix = false;
	}

	@Test
	public void testGettingValueMapContains() {
			assertEquals(map.Get("key").symbol, symbol.symbol);
	}

	@Test
	public void testGettingValueMapDoesntContain() {
			assertEquals(map.Get("yek").symbol, newSymbol.symbol);
	}

}
